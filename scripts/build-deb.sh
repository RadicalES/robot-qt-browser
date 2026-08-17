#!/bin/sh
# Build Debian package for robot-browser
# Usage: ./scripts/build-deb.sh <arch>
#   arch: arm64, armhf, or amd64
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Package metadata
PKG_NAME="robot-browser"
PKG_VERSION=$(cat "${PROJECT_DIR}/VERSION" | tr -d '[:space:]')
PKG_REVISION="1"
PKG_FULL="${PKG_NAME}_${PKG_VERSION}-${PKG_REVISION}"

ARCH="${1:-amd64}"

case "$ARCH" in
    arm64|amd64) ;;
    armhf)
        echo "ERROR: armhf (BeagleBone Black) is retired - QtWebEngine cannot"
        echo "       target linuxfb. The last QtWebKit build is on the 'webkit' branch."
        exit 1 ;;
    *) echo "Usage: $0 <arm64|amd64>"; exit 1 ;;
esac

echo "=== Building robot-browser for ${ARCH} ==="

# Step 1: Build the binary
case "$ARCH" in
    arm64)
        "${PROJECT_DIR}/docker/build-cm4.sh"
        BINARY="${PROJECT_DIR}/build-cm4/robot-browser"
        ;;
    amd64)
        echo "=== Building locally for amd64 ==="
        BUILD_DIR="${PROJECT_DIR}/build-amd64"
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake "${PROJECT_DIR}/src" -DCMAKE_BUILD_TYPE=Release
        cmake --build . -- -j$(nproc)
        BINARY="${BUILD_DIR}/robot-browser"
        cd "$PROJECT_DIR"
        ;;
esac

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

# Step 3: Install files
cp "$BINARY" "$STAGE/usr/bin/robot-browser"
chmod 755 "$STAGE/usr/bin/robot-browser"

cp "${PROJECT_DIR}/debian/robotbrowser.sh" "$STAGE/usr/lib/robot-browser/robotbrowser.sh"
chmod 755 "$STAGE/usr/lib/robot-browser/robotbrowser.sh"

cp "${PROJECT_DIR}/debian/robot-browser.service" "$STAGE/usr/lib/systemd/system/robot-browser.service"
chmod 644 "$STAGE/usr/lib/systemd/system/robot-browser.service"

cp "${PROJECT_DIR}/debian/browser.config" "$STAGE/etc/robot-browser/browser.config"
chmod 644 "$STAGE/etc/robot-browser/browser.config"

cp "${PROJECT_DIR}/rootfs/etc/udev/rules.d/99-robot-input.rules" "$STAGE/etc/udev/rules.d/99-robot-input.rules"
chmod 644 "$STAGE/etc/udev/rules.d/99-robot-input.rules"

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

# Step 4: Calculate installed size (in KB)
INSTALLED_SIZE=$(du -sk "$STAGE" | awk '{print $1}')

# Step 5: Generate DEBIAN/control
cat > "$STAGE/DEBIAN/control" << EOF
Package: ${PKG_NAME}
Version: ${PKG_VERSION}-${PKG_REVISION}
Architecture: ${ARCH}
Maintainer: Radical Electronic Systems <info@radicalsystems.co.za>
Installed-Size: ${INSTALLED_SIZE}
Depends: libqt6core6, libqt6gui6, libqt6widgets6, libqt6network6, libqt6dbus6, libqt6websockets6, libqt6webenginewidgets6, libqt6quickwidgets6, libqt6qml6, libqt6quick6, qt6-virtualkeyboard-plugin, qml6-module-qtquick-virtualkeyboard, qml6-module-qtqml-workerscript, qml6-module-qt-labs-folderlistmodel, network-manager
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
