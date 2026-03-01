#!/bin/sh
# Cross-compile RBrowser for Raspberry Pi CM4 (arm64) inside Docker
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

IMAGE_NAME="rbrowser-cm4-build"

echo "=== Building Docker image ==="
docker build -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile.cm4" "$SCRIPT_DIR"

echo "=== Cross-compiling RBrowser ==="
docker run --rm \
    -v "$PROJECT_DIR:/src:ro" \
    -v "$PROJECT_DIR/build-cm4:/build" \
    "$IMAGE_NAME" \
    sh -c '
        # Redirect qmake to arm64 Qt paths (qt.conf must sit next to real qmake binary)
        cat > /usr/lib/qt5/bin/qt.conf <<QTCONF
[Paths]
Prefix = /usr/lib/aarch64-linux-gnu/qt5
Headers = /usr/include/aarch64-linux-gnu/qt5
Libraries = /usr/lib/aarch64-linux-gnu
QTCONF

        cd /build
        /usr/lib/qt5/bin/qmake /src/RBrowser.pro -spec linux-aarch64-gnu-g++
        make -j$(nproc)
    '

echo "=== Done ==="
echo "Binary: $PROJECT_DIR/build-cm4/RBrowser"
file "$PROJECT_DIR/build-cm4/RBrowser" 2>/dev/null || true
