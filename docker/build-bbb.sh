#!/bin/sh
# Cross-compile RBrowser for BeagleBone Black (armhf) inside Docker
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

IMAGE_NAME="rbrowser-bbb-build"

echo "=== Building Docker image ==="
docker build -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile.bbb" "$SCRIPT_DIR"

echo "=== Cross-compiling RBrowser ==="
docker run --rm \
    -v "$PROJECT_DIR:/src:ro" \
    -v "$PROJECT_DIR/build-bbb:/build" \
    "$IMAGE_NAME" \
    sh -c '
        # Redirect qmake to armhf Qt paths (qt.conf must sit next to real qmake binary)
        cat > /usr/lib/qt5/bin/qt.conf <<QTCONF
[Paths]
Prefix = /usr/lib/arm-linux-gnueabihf/qt5
Headers = /usr/include/arm-linux-gnueabihf/qt5
Libraries = /usr/lib/arm-linux-gnueabihf
QTCONF

        cd /build
        /usr/lib/qt5/bin/qmake /src/src/RBrowser.pro -spec linux-arm-gnueabihf-g++
        make -j$(nproc)
    '

echo "=== Done ==="
echo "Binary: $PROJECT_DIR/build-bbb/RBrowser"
file "$PROJECT_DIR/build-bbb/RBrowser" 2>/dev/null || true
