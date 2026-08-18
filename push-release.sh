#!/bin/bash
# =============================================================================
# Push release commit and tag to origin
# =============================================================================
#
# Usage:
#   ./push-release.sh
#
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="$SCRIPT_DIR/VERSION"

VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')
BRANCH=$(git rev-parse --abbrev-ref HEAD)
# Releases from the release branch are tagged plain vX.Y.Z; everything else is
# branch-prefixed, so a pre-release tag can never be mistaken for a release.
# Both branch names are accepted while the rename from master to main is
# outstanding.
case "$BRANCH" in
    main|master) TAG="v${VERSION}" ;;
    *)           TAG="${BRANCH}-v${VERSION}" ;;
esac

echo "Version:  $VERSION"
echo "Branch:   $BRANCH"
echo "Tag:      $TAG"
echo ""

# Verify tag exists locally
if ! git tag -l "$TAG" | grep -q "$TAG"; then
    echo "ERROR: Tag '$TAG' does not exist locally"
    echo "Run ./release.sh first"
    exit 1
fi

echo "Pushing branch '$BRANCH' and tag '$TAG' to origin..."
git push origin "$BRANCH"
git push origin "$TAG"

echo ""
echo "Done."
