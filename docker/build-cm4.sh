#!/bin/sh
# Cross-compile robot-browser for Raspberry Pi CM4 / Pi 5 (arm64) inside Docker.
#
# Qt 6 + QtWebEngine. Uses CMake rather than qmake: Debian's Qt 6 ships no
# linux-aarch64-gnu-g++ mkspec, so the qmake + qt.conf redirect this script
# previously used for Qt 5 cannot work.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

IMAGE_NAME="rbrowser-cm4-qt6-build"
OUT_DIR="$PROJECT_DIR/build-cm4"

echo "=== Building Docker image ==="
docker build -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile.cm4-qt6" "$SCRIPT_DIR"

mkdir -p "$OUT_DIR"

echo "=== Cross-compiling robot-browser (arm64) ==="
docker run --rm \
    -v "$PROJECT_DIR:/src:ro" \
    -v "$OUT_DIR:/build" \
    "$IMAGE_NAME" \
    sh -c '
        cd /build
        cmake /src/src \
            -G Ninja \
            -DCMAKE_TOOLCHAIN_FILE=/src/docker/toolchain-arm64.cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DQT_HOST_PATH=/usr
        cmake --build . -- -j"$(nproc)"
    '

echo "=== Done ==="
echo "Binary: $OUT_DIR/robot-browser"
file "$OUT_DIR/robot-browser" 2>/dev/null || true
