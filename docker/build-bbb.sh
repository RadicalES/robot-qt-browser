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
        cp -r /src/* /build/ 2>/dev/null || true
        cd /build
        qmake /src/RBrowser.pro \
            -spec linux-arm-gnueabihf-g++ \
            "QMAKE_CC=arm-linux-gnueabihf-gcc" \
            "QMAKE_CXX=arm-linux-gnueabihf-g++" \
            "QMAKE_LINK=arm-linux-gnueabihf-g++" \
            "QMAKE_LINK_SHLIB=arm-linux-gnueabihf-g++"
        make -j$(nproc)
    '

echo "=== Done ==="
echo "Binary: $PROJECT_DIR/build-bbb/RBrowser"
file "$PROJECT_DIR/build-bbb/RBrowser" 2>/dev/null || true
