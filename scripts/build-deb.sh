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
    arm64|armhf|amd64) ;;
    *) echo "Usage: $0 <arm64|armhf|amd64>"; exit 1 ;;
esac

echo "=== Building robot-browser for ${ARCH} ==="

# Step 1: Build the binary
case "$ARCH" in
    arm64)
        "${PROJECT_DIR}/docker/build-cm4.sh"
        BINARY="${PROJECT_DIR}/build-cm4/robot-browser"
        ;;
    armhf)
        "${PROJECT_DIR}/docker/build-bbb.sh"
        BINARY="${PROJECT_DIR}/build-bbb/robot-browser"
        ;;
    amd64)
        echo "=== Building locally for amd64 ==="
        BUILD_DIR="${PROJECT_DIR}/build-amd64"
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        qmake "${PROJECT_DIR}/src/robot-browser.pro"
        make -j$(nproc)
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
Depends: libqt5core5a, libqt5gui5, libqt5widgets5, libqt5network5, libqt5qml5, libqt5quick5, libqt5quickwidgets5, libqt5quickcontrols2-5, libqt5quicktemplates2-5, libqt5webkit5, libqt5websockets5, libqt5virtualkeyboard5, libqt5dbus5, qml-module-qtquick2, qml-module-qtquick-controls2, qml-module-qtquick-layouts, qml-module-qtquick-window2, qtvirtualkeyboard-plugin, network-manager
Section: misc
Priority: optional
Description: Kiosk browser for embedded Linux robots
 Two-URL kiosk browser with WiFi management, system info, and virtual
 keyboard. Built with Qt 5.15 and QtWebKit 5.212 for BeagleBone Black,
 Raspberry Pi CM4, and x86 platforms.
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
