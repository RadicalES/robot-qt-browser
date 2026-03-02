#!/bin/bash
# Bump version number in VERSION file
#
# Usage:
#   ./scripts/bump-version.sh patch     # 2.1.0 → 2.1.1
#   ./scripts/bump-version.sh minor     # 2.1.0 → 2.2.0
#   ./scripts/bump-version.sh major     # 2.1.0 → 3.0.0
#   ./scripts/bump-version.sh 2.5.0     # Set explicit version

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
VERSION_FILE="$ROOT_DIR/VERSION"

if [ ! -f "$VERSION_FILE" ]; then
    echo "ERROR: VERSION file not found at $VERSION_FILE"
    exit 1
fi

CURRENT=$(cat "$VERSION_FILE" | tr -d '[:space:]')
echo "Current version: $CURRENT"

# Parse current version
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT"

BUMP_TYPE="${1:-patch}"

case "$BUMP_TYPE" in
    major)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
    minor)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    patch)
        PATCH=$((PATCH + 1))
        ;;
    *)
        # Check if it's a valid semver
        if [[ "$BUMP_TYPE" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            IFS='.' read -r MAJOR MINOR PATCH <<< "$BUMP_TYPE"
        else
            echo "ERROR: Invalid bump type '$BUMP_TYPE'"
            echo "Usage: $0 [major|minor|patch|X.Y.Z]"
            exit 1
        fi
        ;;
esac

NEW_VERSION="$MAJOR.$MINOR.$PATCH"
echo "$NEW_VERSION" > "$VERSION_FILE"
echo "New version: $NEW_VERSION"

echo ""
echo "Next steps:"
echo "  1. Test your changes"
echo "  2. Run: ./release.sh"
echo "  3. Run: ./push-release.sh"
