#!/bin/bash
#
# publish-deb.sh - put a built package into the CDN's pool and publish it.
#
# This project's side of publishing is only ever "copy the .deb into the pool
# and ask the package repository to publish". Metadata, signing and the R2
# sync belong to that repository, so they live there and are called, not
# reimplemented here.
#
# Packages on the CDN are what terminals install by apt, so this refuses to run
# anywhere but the release branch. Test builds go onto a device by hand — see
# WORKFLOW.md, "Manual install".
#
# Usage:
#   ./scripts/publish-deb.sh [arm64|amd64]     # default arm64
#   ./scripts/publish-deb.sh --dry-run
#
# Environment:
#   PACKAGE_REPO   path to the package repository checkout
#                  (default: /home/janz/data/package-repository)
#
# (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE_REPO="${PACKAGE_REPO:-/home/janz/data/package-repository}"
PACKAGE="robot-browser"

ARCH="arm64"
DRY_RUN=false
for arg in "$@"; do
    case "$arg" in
        arm64|amd64) ARCH="$arg" ;;
        --dry-run)   DRY_RUN=true ;;
        -h|--help)   sed -n '3,20p' "${BASH_SOURCE[0]}" | sed 's/^# \?//'; exit 0 ;;
        *)           echo "ERROR: unknown option '$arg'"; exit 1 ;;
    esac
done

VERSION=$(tr -d '[:space:]' < "${PROJECT_DIR}/VERSION")
BRANCH=$(git -C "$PROJECT_DIR" rev-parse --abbrev-ref HEAD)
# The package version carries the suite it was built for — 3.4.0-1~bookworm —
# because a build made against one distro's Qt does not run on another's. The
# exact suffix comes from the build, so it is matched rather than assumed.
DEB=$(ls -1 "${PROJECT_DIR}/build-deb/${PACKAGE}_${VERSION}-1"*"_${ARCH}.deb" 2>/dev/null | head -1)
[ -n "$DEB" ] || DEB="${PROJECT_DIR}/build-deb/${PACKAGE}_${VERSION}-1_${ARCH}.deb"
POOL="${PACKAGE_REPO}/debian/pool/main/${ARCH}"

echo "Package:  ${PACKAGE} ${VERSION}-1 (${ARCH})"
echo "Branch:   ${BRANCH}"
echo "Pool:     ${POOL}"
echo ""

# --- Checks -----------------------------------------------------------------

if [ "$BRANCH" != "main" ] && [ "$BRANCH" != "master" ]; then
    echo "ERROR: publishing is only allowed from the release branch."
    echo "You are on '${BRANCH}'. Promote first:"
    echo "  git checkout beta && git merge dev && git push origin beta"
    echo "  git checkout main && git merge beta && git push origin main"
    exit 1
fi

if [ -n "$(git -C "$PROJECT_DIR" status --porcelain)" ]; then
    echo "ERROR: working tree is not clean — commit or stash first."
    exit 1
fi

if ! git -C "$PROJECT_DIR" tag -l | grep -qx "v${VERSION}"; then
    echo "ERROR: version ${VERSION} is not tagged. Release it first:"
    echo "  ./release.sh && ./push-release.sh"
    exit 1
fi

if [ ! -d "$POOL" ]; then
    echo "ERROR: pool directory not found: ${POOL}"
    echo "Set PACKAGE_REPO to the package repository checkout."
    exit 1
fi

if [ ! -f "$DEB" ]; then
    echo "Package not built yet — building."
    "${PROJECT_DIR}/scripts/build-deb.sh" "$ARCH"
    DEB=$(ls -1 "${PROJECT_DIR}/build-deb/${PACKAGE}_${VERSION}-1"*"_${ARCH}.deb" 2>/dev/null | head -1)
    if [ -z "$DEB" ]; then
        echo "ERROR: no package built for ${ARCH} at version ${VERSION}"
        exit 1
    fi
fi

echo "Package file: $(basename "$DEB")"

# --- Publish ----------------------------------------------------------------

# Only one version of a package belongs in the pool: apt offers the newest it
# is told about, and leaving the old file behind means it stays on the CDN and
# in the metadata for a version nobody should install.
OLD=$(find "$POOL" -maxdepth 1 -name "${PACKAGE}_*_${ARCH}.deb" \
        ! -name "${PACKAGE}_${VERSION}-1_${ARCH}.deb")

if [ "$DRY_RUN" = true ]; then
    echo "[DRY RUN] Would copy:   $(basename "$DEB") -> ${POOL}/"
    [ -n "$OLD" ] && echo "$OLD" | while read -r f; do
        echo "[DRY RUN] Would remove: $(basename "$f")"
    done
    echo ""
    cd "$PACKAGE_REPO" && exec ./scripts/publish.sh --dry-run
fi

cp "$DEB" "${POOL}/"
echo "Copied:  $(basename "$DEB")"
[ -n "$OLD" ] && echo "$OLD" | while read -r f; do
    rm -f "$f"
    echo "Removed: $(basename "$f")"
done

echo ""
cd "$PACKAGE_REPO"
./scripts/publish.sh
