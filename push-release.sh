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
TAG="${BRANCH}-v${VERSION}"

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
