#!/bin/bash
# =============================================================================
# Draft release notes, commit, and tag
# =============================================================================
#
# Usage:
#   ./release.sh              # Draft release notes, open editor, commit & tag
#   ./release.sh --dry-run    # Preview without modifying anything
#
# Tag format: {branch}-v{version}  (e.g., dev-v2.1.0, beta-v2.1.0, master-v2.1.0)
#
# =============================================================================

set -e

DRY_RUN=false
if [ "$1" = "--dry-run" ]; then
    DRY_RUN=true
    echo "=== DRY RUN MODE ==="
    echo ""
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="$SCRIPT_DIR/VERSION"
RELEASE_FILE="$SCRIPT_DIR/RELEASE.md"

if [ ! -f "$VERSION_FILE" ]; then
    echo "ERROR: VERSION file not found"
    exit 1
fi

VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')
BRANCH=$(git rev-parse --abbrev-ref HEAD)
TAG="${BRANCH}-v${VERSION}"
DATE=$(date +%Y-%m-%d)

echo "Version:  $VERSION"
echo "Branch:   $BRANCH"
echo "Tag:      $TAG"
echo ""

# Check if tag already exists
if git tag -l "$TAG" | grep -q "$TAG"; then
    echo "ERROR: Tag '$TAG' already exists"
    echo "Bump the version first: ./scripts/bump-version.sh"
    exit 1
fi

# Collect commits since last tag on this branch
LAST_TAG=$(git describe --tags --abbrev=0 --match "${BRANCH}-v*" 2>/dev/null || echo "")
if [ -n "$LAST_TAG" ]; then
    echo "Changes since $LAST_TAG:"
    COMMITS=$(git log --oneline "$LAST_TAG"..HEAD)
else
    echo "Changes (last 20 commits):"
    COMMITS=$(git log --oneline -20)
fi
echo "$COMMITS"
echo ""

# Check if entry already in RELEASE.md
if grep -q "## \[${VERSION}\]" "$RELEASE_FILE" 2>/dev/null; then
    echo "Release notes for $VERSION already in RELEASE.md"
    echo ""
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Would skip to tagging (notes already exist)"
    else
        read -p "Skip to tagging? [y/N] " SKIP
        if [ "$SKIP" != "y" ] && [ "$SKIP" != "Y" ]; then
            echo "Aborted."
            exit 0
        fi
    fi
else
    # Draft release notes
    DRAFT=$(mktemp)
    {
        echo "## [$VERSION] - $DATE"
        echo ""
        echo "### Changed"
        echo "$COMMITS" | while IFS= read -r line; do
            echo "- $line"
        done
        echo ""
    } > "$DRAFT"

    echo "=== Draft release notes ==="
    cat "$DRAFT"
    echo "==========================="
    echo ""

    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Would insert into RELEASE.md and open editor"
        rm "$DRAFT"
        exit 0
    fi

    # Insert after the --- separator (line after the header block)
    MARKER_LINE=$(grep -n "^---$" "$RELEASE_FILE" | head -1 | cut -d: -f1)
    if [ -z "$MARKER_LINE" ]; then
        echo "ERROR: Could not find --- marker in RELEASE.md"
        rm "$DRAFT"
        exit 1
    fi

    # Build new RELEASE.md: header + draft + rest
    TEMP_RELEASE=$(mktemp)
    head -n "$MARKER_LINE" "$RELEASE_FILE" > "$TEMP_RELEASE"
    echo "" >> "$TEMP_RELEASE"
    cat "$DRAFT" >> "$TEMP_RELEASE"
    tail -n +$((MARKER_LINE + 1)) "$RELEASE_FILE" >> "$TEMP_RELEASE"
    mv "$TEMP_RELEASE" "$RELEASE_FILE"
    rm "$DRAFT"

    # Open editor for review
    EDIT="${EDITOR:-${VISUAL:-vi}}"
    echo "Opening $EDIT to review release notes..."
    $EDIT "$RELEASE_FILE"
fi

# Show what will be committed
echo ""
echo "=== Release notes for $VERSION ==="
awk "/^## \[${VERSION}\]/{found=1; print; next} found && /^(## \[|---)/{exit} found{print}" "$RELEASE_FILE"
echo "==================================="
echo ""

if [ "$DRY_RUN" = true ]; then
    echo "[DRY RUN] Would commit and tag as $TAG"
    exit 0
fi

read -p "Commit and tag as '$TAG'? [y/N] " CONFIRM
if [ "$CONFIRM" != "y" ] && [ "$CONFIRM" != "Y" ]; then
    echo "Aborted. RELEASE.md has been updated but not committed."
    exit 0
fi

# Commit (if there are changes) and tag
git add "$RELEASE_FILE"
if ! git diff --cached --quiet; then
    git commit -m "Release $VERSION"
else
    echo "No changes to commit (release notes already up to date)"
fi
git tag -a "$TAG" -m "Release $VERSION"

echo ""
echo "Tagged: $TAG"
echo "Next: ./push-release.sh"
