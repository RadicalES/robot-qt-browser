#!/bin/sh
# Run robot-browser for one target on this machine, from its own .deb.
#
# Compiling proves nothing about running. Everything that has gone wrong on the
# terminals went wrong at runtime: QtWebEngine needing a GPU and a compositor,
# the virtual keyboard's QML modules missing from the package's Depends and
# leaving a dead panel filling half the screen, the keyboard binding to the
# active window rather than the focused widget. None of it is visible to a
# compiler.
#
# So this does not run the binary out of the build tree. It installs the built
# package with apt, inside that target's own distro, which puts the package's
# dependency list under test as well — and then puts a window on your desktop.
#
# Usage:
#   ./docker/run-desktop.sh bookworm
#   ./docker/run-desktop.sh trixie --profile=t440 https://example.com http://localhost:3000
#   ./docker/run-desktop.sh ubuntu-26.04 --check      # install only, no window
#
# Everything after the target is passed to robot-browser. --check stops after
# the install, which is the half worth running in CI: it proves the package's
# Depends resolve on that distro, which is where they differ.
#
# Environment:
#   ROBOT_BUILD_VOL   volume root (default /home/janz/data1tb/var/containers/robot-browser)
#
# (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_VOL="${ROBOT_BUILD_VOL:-/home/janz/data1tb/var/containers/robot-browser}"

TARGET="${1:-}"
[ -n "$TARGET" ] || { echo "Usage: $0 <bookworm|trixie|ubuntu-24.04|ubuntu-26.04> [--check] [browser args...]"; exit 1; }
shift

CHECK_ONLY=false
if [ "${1:-}" = "--check" ]; then
    CHECK_ONLY=true
    shift
fi

case "$TARGET" in
    bookworm)     BASE="debian:12" ;;
    trixie)       BASE="debian:13" ;;
    ubuntu-24.04) BASE="ubuntu:24.04" ;;
    ubuntu-26.04) BASE="ubuntu:26.04" ;;
    *) echo "ERROR: unknown target '$TARGET'"; exit 1 ;;
esac

VERSION=$(tr -d '[:space:]' < "${PROJECT_DIR}/VERSION")
SUITE=$(cat "${PROJECT_DIR}/build-${TARGET}/SUITE" 2>/dev/null || echo "")
DEB="${PROJECT_DIR}/build-deb/robot-browser_${VERSION}-1~${SUITE}_amd64.deb"

if [ ! -f "$DEB" ]; then
    echo "=== No package for $TARGET yet — building it ==="
    "${PROJECT_DIR}/scripts/build-deb.sh" "$TARGET"
    SUITE=$(cat "${PROJECT_DIR}/build-${TARGET}/SUITE")
    DEB="${PROJECT_DIR}/build-deb/robot-browser_${VERSION}-1~${SUITE}_amd64.deb"
fi

echo "=== Runtime image for $TARGET (installing $(basename "$DEB")) ==="
RUNTIME_DIR="${BUILD_VOL}/runtime/${TARGET}"
mkdir -p "$RUNTIME_DIR"
cp "$DEB" "$RUNTIME_DIR/robot-browser.deb"

# A plain base image plus the package, so apt resolves Depends exactly as it
# would on a customer's machine. Nothing from the build environment is here:
# if a runtime dependency is missing from debian/control, it fails here rather
# than on the device.
cat > "$RUNTIME_DIR/Dockerfile" <<'DOCKEREOF'
ARG BASE_IMAGE
FROM ${BASE_IMAGE}
ENV DEBIAN_FRONTEND=noninteractive
COPY robot-browser.deb /tmp/robot-browser.deb
RUN apt-get update && \
    apt-get install -y --no-install-recommends /tmp/robot-browser.deb && \
    rm -rf /var/lib/apt/lists/* /tmp/robot-browser.deb
DOCKEREOF

docker build --build-arg "BASE_IMAGE=$BASE" -t "rbrowser-run-${TARGET}" "$RUNTIME_DIR"

if [ "$CHECK_ONLY" = true ]; then
    echo "=== [$TARGET] Package installs on $BASE, with its dependencies satisfied ==="
    docker run --rm "rbrowser-run-${TARGET}" sh -c \
        'robot-browser --profile=nonexistent 2>&1 | head -1; dpkg -s robot-browser | grep -E "^(Version|Status)"'
    exit 0
fi

# Display. This workstation runs Wayland with Xwayland, and Qt's xcb platform
# through the X socket is also what the terminals use — the kiosk session runs
# Xwayland precisely because Qt 6 refuses the virtual keyboard on Wayland.
XSOCK=/tmp/.X11-unix
XAUTH_ARGS=""
if [ -n "$XAUTHORITY" ] && [ -r "$XAUTHORITY" ]; then
    XAUTH_ARGS="-v $XAUTHORITY:/tmp/.Xauth:ro -e XAUTHORITY=/tmp/.Xauth"
fi

echo "=== Running ==="
# --device /dev/dri: QtWebEngine wants the GPU, and software rendering is not
#   what the terminals do, so testing on it would prove the wrong thing.
# --ipc=host: Chromium's shared memory. Without it pages render blank or crash.
# QTWEBENGINE_DISABLE_SANDBOX: Chromium's sandbox needs privileges a container
#   does not have by default. This is a local test harness, not a deployment —
#   the packaged terminals run sandboxed.
# Interactive only when there is a terminal to be interactive with, so this is
# also usable from a script or a CI job.
TTY_ARGS="-i"
[ -t 0 ] && TTY_ARGS="-it"

exec docker run --rm $TTY_ARGS \
    -e DISPLAY="${DISPLAY:-:0}" \
    -e LANG=C.UTF-8 \
    -e QT_QPA_PLATFORM=xcb \
    -e QTWEBENGINE_DISABLE_SANDBOX=1 \
    -v "$XSOCK:$XSOCK" \
    $XAUTH_ARGS \
    --device /dev/dri \
    --ipc=host \
    "rbrowser-run-${TARGET}" \
    robot-browser "$@"
