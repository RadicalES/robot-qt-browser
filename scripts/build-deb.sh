#!/bin/sh
# Build Debian package for robot-browser
#
# Usage: ./scripts/build-deb.sh <target>
#
#   arm64                 Pi CM4 / Pi 5, cross-built against Debian 12's Qt
#   bookworm              Debian 12 amd64, in a container
#   trixie                Debian 13 amd64, in a container
#   ubuntu-24.04          Ubuntu 24.04 amd64, in a container
#   ubuntu-26.04          Ubuntu 26.04 amd64, in a container
#   amd64                 whatever Qt is installed on THIS machine (dev only)
#
# A desktop target is a distro, not an architecture. Each ships its own Qt 6 and
# QtWebEngine and a binary built against one does not run on another, so the
# package version carries the suite it was built for — 3.3.1-1~trixie — and apt
# on a terminal or a PC installs the one built for it.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Package metadata
PKG_NAME="robot-browser"
PKG_VERSION=$(cat "${PROJECT_DIR}/VERSION" | tr -d '[:space:]')
PKG_REVISION="1"
PKG_FULL="${PKG_NAME}_${PKG_VERSION}-${PKG_REVISION}"

TARGET="${1:-amd64}"
SUITE=""

case "$TARGET" in
    armhf)
        echo "ERROR: armhf (BeagleBone Black) is retired - QtWebEngine cannot"
        echo "       target linuxfb. The last QtWebKit build is on the 'webkit' branch."
        exit 1 ;;
    ubuntu-22.04|jammy)
        echo "ERROR: Ubuntu 22.04 ships Qt 6.2 and the floor is Qt 6.4."
        exit 1 ;;
esac

echo "=== Building robot-browser for ${TARGET} ==="

# Step 1: Build the binary
case "$TARGET" in
    arm64)
        "${PROJECT_DIR}/docker/build-cm4.sh"
        BINARY="${PROJECT_DIR}/build-cm4/robot-browser"
        ARCH="arm64"
        # The terminals run Debian 12, and this is cross-built against its Qt.
        SUITE="bookworm"
        ;;
    bookworm|trixie|ubuntu-24.04|ubuntu-26.04)
        "${PROJECT_DIR}/docker/build-desktop.sh" "$TARGET"
        BINARY="${PROJECT_DIR}/build-${TARGET}/robot-browser"
        # Read back from the build, not assumed: the container reports the
        # codename its own /etc/os-release gives, so a release whose codename
        # we guessed cannot be mislabelled.
        SUITE="$(cat "${PROJECT_DIR}/build-${TARGET}/SUITE" 2>/dev/null)"
        ARCH="$(cat "${PROJECT_DIR}/build-${TARGET}/ARCH" 2>/dev/null || echo amd64)"
        ;;
    amd64)
        echo "=== Building locally for amd64 ==="
        echo "    (against this machine's Qt - for development, not for release)"
        BUILD_DIR="${PROJECT_DIR}/build-amd64"
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake "${PROJECT_DIR}/src" -DCMAKE_BUILD_TYPE=Release
        cmake --build . -- -j$(nproc)
        BINARY="${BUILD_DIR}/robot-browser"
        ARCH="amd64"
        cd "$PROJECT_DIR"
        ;;
    *)
        echo "Usage: $0 <arm64|bookworm|trixie|ubuntu-24.04|ubuntu-26.04|amd64>"
        exit 1 ;;
esac

# A suite-qualified version, so the four desktop builds can coexist in the
# repository and apt offers each machine the one built against its own Qt.
if [ -n "$SUITE" ] && [ "$SUITE" != "unknown" ]; then
    PKG_REVISION="${PKG_REVISION}~${SUITE}"
    PKG_FULL="${PKG_NAME}_${PKG_VERSION}-${PKG_REVISION}"
fi

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found at $BINARY"
    exit 1
fi

echo "=== Packaging ${PKG_FULL}_${ARCH}.deb ==="

# Step 2: Create staging directory
STAGE="${PROJECT_DIR}/build-deb/${PKG_FULL}_${ARCH}"
rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN"
mkdir -p "$STAGE/usr/bin"
mkdir -p "$STAGE/usr/lib/robot-browser"
mkdir -p "$STAGE/usr/lib/systemd/system"
mkdir -p "$STAGE/etc/robot-browser"
mkdir -p "$STAGE/etc/udev/rules.d"
# Menu entry and its icon. Global, in /usr/share — a PC install should put the
# browser in the applications menu for everyone on the machine, not for
# whoever ran apt.
mkdir -p "$STAGE/usr/share/applications"
mkdir -p "$STAGE/usr/share/icons/hicolor/scalable/apps"

# Step 3: Install files
cp "$BINARY" "$STAGE/usr/bin/robot-browser"
chmod 755 "$STAGE/usr/bin/robot-browser"

cp "${PROJECT_DIR}/debian/robotbrowser.sh" "$STAGE/usr/lib/robot-browser/robotbrowser.sh"
chmod 755 "$STAGE/usr/lib/robot-browser/robotbrowser.sh"

cp "${PROJECT_DIR}/debian/robot-browser.service" "$STAGE/usr/lib/systemd/system/robot-browser.service"
chmod 644 "$STAGE/usr/lib/systemd/system/robot-browser.service"

cp "${PROJECT_DIR}/debian/browser.config" "$STAGE/etc/robot-browser/browser.config"
chmod 644 "$STAGE/etc/robot-browser/browser.config"

cp "${PROJECT_DIR}/packaging/udev/99-robot-input.rules" "$STAGE/etc/udev/rules.d/99-robot-input.rules"
chmod 644 "$STAGE/etc/udev/rules.d/99-robot-input.rules"

# The entry is named for the application ID, reverse-DNS, which is what a
# desktop matches a window to and what it keys its own cache on.
#
# It was robot-browser.desktop until 3.5.0, and the rename is deliberate:
# GNOME Shell caches an app by that id for the life of a session, so a machine
# that had seen an older entry kept launching the old Exec and drawing the old
# icon no matter what the package installed - for months, on a workstation that
# is never logged out. A new id is one the shell has never seen.
DESKTOP_ID="za.co.radicalsystems.RobotBrowser"
cp "${PROJECT_DIR}/packaging/desktop/${DESKTOP_ID}.desktop" "$STAGE/usr/share/applications/${DESKTOP_ID}.desktop"
chmod 644 "$STAGE/usr/share/applications/${DESKTOP_ID}.desktop"
cp "${PROJECT_DIR}/src/images/robot-head.svg" "$STAGE/usr/share/icons/hicolor/scalable/apps/robot-browser.svg"
chmod 644 "$STAGE/usr/share/icons/hicolor/scalable/apps/robot-browser.svg"

# Custom virtual keyboard style (red lettering, from the T420 terminal).
# Must sit at QtQuick/VirtualKeyboard/Styles/<name> under a QML import root.
if [ -d "${PROJECT_DIR}/styles/robot" ]; then
    STYLE_DIR="$STAGE/usr/share/robot-browser/qml/QtQuick/VirtualKeyboard/Styles/robot"
    mkdir -p "$STYLE_DIR"
    cp -r "${PROJECT_DIR}/styles/robot/"* "$STYLE_DIR/"
fi

# Virtual keyboard layouts
if [ -d "${PROJECT_DIR}/layouts" ]; then
    mkdir -p "$STAGE/usr/share/robot-browser/layouts"
    cp -r "${PROJECT_DIR}/layouts/"* "$STAGE/usr/share/robot-browser/layouts/"
fi

# The alternative layouts — narrow (four across, side-docked) and full (twelve
# across with numbers and symbols on one page) — chosen at runtime by pointing
# QT_VIRTUALKEYBOARD_LAYOUT_PATH at one of them.
for variant in layouts-narrow layouts-full; do
    if [ -d "${PROJECT_DIR}/${variant}" ]; then
        mkdir -p "$STAGE/usr/share/robot-browser/${variant}"
        cp -r "${PROJECT_DIR}/${variant}/"* "$STAGE/usr/share/robot-browser/${variant}/"
    fi
done

# Step 4: Calculate installed size (in KB)
INSTALLED_SIZE=$(du -sk "$STAGE" | awk '{print $1}')

# Step 5: Generate DEBIAN/control
cat > "$STAGE/DEBIAN/control" << EOF
Package: ${PKG_NAME}
Version: ${PKG_VERSION}-${PKG_REVISION}
Architecture: ${ARCH}
Maintainer: Radical Electronic Systems <info@radicalsystems.co.za>
Installed-Size: ${INSTALLED_SIZE}
Depends: libqt6core6, libqt6gui6, libqt6widgets6, libqt6network6, libqt6dbus6, libqt6websockets6, libqt6svg6, libqt6webenginewidgets6, libqt6quickwidgets6, libqt6qml6, libqt6quick6, qt6-virtualkeyboard-plugin, qml6-module-qtquick-virtualkeyboard, qml6-module-qtqml-workerscript, qml6-module-qt-labs-folderlistmodel, qml6-module-qtquick-layouts, qml6-module-qtquick-window, qml6-module-qtquick-controls, qml6-module-qtquick-templates, network-manager
Section: misc
Priority: optional
Description: Kiosk browser for embedded Linux robots
 Two-URL kiosk browser with WiFi management, system info, and virtual
 keyboard. Built with Qt 6 and QtWebEngine for Raspberry Pi CM4 and
 Raspberry Pi 5.
EOF

# Step 6: Install maintainer scripts
for script in postinst prerm postrm; do
    if [ -f "${PROJECT_DIR}/debian/${script}" ]; then
        cp "${PROJECT_DIR}/debian/${script}" "$STAGE/DEBIAN/${script}"
        chmod 755 "$STAGE/DEBIAN/${script}"
    fi
done

# Step 7: Conffiles
cp "${PROJECT_DIR}/debian/conffiles" "$STAGE/DEBIAN/conffiles"

# Step 8: Build the .deb
OUTPUT_DIR="${PROJECT_DIR}/build-deb"
dpkg-deb --build --root-owner-group "$STAGE" "${OUTPUT_DIR}/${PKG_FULL}_${ARCH}.deb"

echo "=== Done ==="
echo "Package: ${OUTPUT_DIR}/${PKG_FULL}_${ARCH}.deb"
dpkg-deb -I "${OUTPUT_DIR}/${PKG_FULL}_${ARCH}.deb"
