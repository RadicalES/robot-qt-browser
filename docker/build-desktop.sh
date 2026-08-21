#!/bin/sh
# Build robot-browser for one desktop target, inside that target's own distro.
#
# A target is a distro, not an architecture: bookworm, trixie, Ubuntu 24.04 and
# Ubuntu 26.04 each ship a different Qt 6 and a different QtWebEngine, and a
# binary built against one does not run on another. So each gets a container
# holding that distro's Qt, and nothing is built against "whatever is installed
# on this machine".
#
# Usage:
#   ./docker/build-desktop.sh bookworm|trixie|ubuntu-24.04|ubuntu-26.04
#   ./docker/build-desktop.sh all
#
# Environment:
#   ROBOT_BUILD_VOL   where build trees and the compiler cache live
#                     (default /home/janz/data1tb/var/containers/robot-browser)
#
# The object trees are large and rebuilt often, so they are kept on the data
# drive beside the other container volumes rather than in the source tree or on
# the root filesystem. Only the finished binary is copied back into the project.
#
# (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_VOL="${ROBOT_BUILD_VOL:-/home/janz/data1tb/var/containers/robot-browser}"

# Target -> base image. The suite name for the package version is NOT listed
# here: it is read from the image's own /etc/os-release during the build, so a
# release whose codename we guessed wrong cannot be mislabelled.
base_image_for() {
    case "$1" in
        bookworm)     echo "debian:12" ;;
        trixie)       echo "debian:13" ;;
        ubuntu-24.04) echo "ubuntu:24.04" ;;
        ubuntu-26.04) echo "ubuntu:26.04" ;;
        *)            echo "" ;;
    esac
}

ALL_TARGETS="bookworm trixie ubuntu-24.04 ubuntu-26.04"

build_one() {
    TARGET="$1"
    BASE="$(base_image_for "$TARGET")"
    if [ -z "$BASE" ]; then
        echo "ERROR: unknown target '$TARGET'"
        echo "Known: $ALL_TARGETS"
        echo ""
        echo "Ubuntu 22.04 is not a target: it ships Qt 6.2 and the floor is 6.4."
        exit 1
    fi

    IMAGE="rbrowser-build-${TARGET}"
    BUILD_DIR="${BUILD_VOL}/build/${TARGET}"
    CCACHE_DIR="${BUILD_VOL}/ccache/${TARGET}"
    OUT_DIR="${PROJECT_DIR}/build-${TARGET}"

    mkdir -p "$BUILD_DIR" "$CCACHE_DIR" "$OUT_DIR"

    echo "=== [$TARGET] Build environment ($BASE) ==="
    docker build \
        --build-arg "BASE_IMAGE=$BASE" \
        -t "$IMAGE" \
        -f "$SCRIPT_DIR/Dockerfile.desktop" \
        "$SCRIPT_DIR"

    echo "=== [$TARGET] Compiling ==="
    docker run --rm \
        -v "$PROJECT_DIR:/src:ro" \
        -v "$BUILD_DIR:/build" \
        -v "$CCACHE_DIR:/ccache" \
        "$IMAGE" \
        sh -c '
            set -e
            cd /build
            cmake /src/src -G Ninja -DCMAKE_BUILD_TYPE=Release
            cmake --build . -- -j"$(nproc)"
            # The suite this was built against, from the image itself rather
            # than from a table someone has to keep correct.
            . /etc/os-release
            printf "%s\n" "${VERSION_CODENAME:-unknown}" > /build/SUITE
            printf "%s\n" "$(dpkg --print-architecture)" > /build/ARCH
        '

    cp "$BUILD_DIR/robot-browser" "$OUT_DIR/robot-browser"
    cp "$BUILD_DIR/SUITE" "$BUILD_DIR/ARCH" "$OUT_DIR/"

    echo "=== [$TARGET] Done ==="
    echo "  Binary: $OUT_DIR/robot-browser"
    echo "  Suite:  $(cat "$OUT_DIR/SUITE")   Arch: $(cat "$OUT_DIR/ARCH")"
    file "$OUT_DIR/robot-browser" 2>/dev/null || true
    echo ""
}

case "${1:-}" in
    "")    echo "Usage: $0 <$(echo $ALL_TARGETS | tr ' ' '|')|all>"; exit 1 ;;
    all)   for t in $ALL_TARGETS; do build_one "$t"; done ;;
    *)     build_one "$1" ;;
esac
