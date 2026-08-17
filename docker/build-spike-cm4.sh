#!/bin/sh
# Cross-compile the Phase 0 QtWebEngine spike for CM4 / Pi 5 (arm64) in Docker.
#
# Throwaway build for spike/webengine-cm4 — see that directory's README.
# Uses CMake, not qmake: Debian's Qt 6 has no aarch64 mkspec.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

IMAGE_NAME="rbrowser-cm4-qt6-build"
OUT_DIR="$PROJECT_DIR/build-spike-cm4"

echo "=== Building Docker image ==="
docker build -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile.cm4-qt6" "$SCRIPT_DIR"

mkdir -p "$OUT_DIR"

echo "=== Cross-compiling webengine-spike (arm64) ==="
docker run --rm \
    -v "$PROJECT_DIR:/src:ro" \
    -v "$OUT_DIR:/build" \
    "$IMAGE_NAME" \
    sh -c '
        cd /build
        cmake /src/spike/webengine-cm4 \
            -G Ninja \
            -DCMAKE_TOOLCHAIN_FILE=/src/docker/toolchain-arm64.cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DQT_HOST_PATH=/usr
        cmake --build . -- -j"$(nproc)"
    '

echo "=== Done ==="
echo "Binary: $OUT_DIR/webengine-spike"
file "$OUT_DIR/webengine-spike" 2>/dev/null || true
echo
echo "Copy to the device and run there:"
echo "  scp $OUT_DIR/webengine-spike spike/webengine-cm4/measure.sh <device>:~/"
