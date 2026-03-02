#!/bin/sh
# (C) 2017-2026, Radical Electronic Systems
# Install robot-browser and runtime dependencies on Debian 12
# Run as root on the target device
set -e

echo "=== Installing Qt 5.15 runtime packages ==="
apt-get update
apt-get install -y --no-install-recommends \
    libqt5core5a \
    libqt5gui5 \
    libqt5widgets5 \
    libqt5network5 \
    libqt5qml5 \
    libqt5quick5 \
    libqt5quickwidgets5 \
    libqt5quickcontrols2-5 \
    libqt5quicktemplates2-5 \
    libqt5webkit5 \
    libqt5websockets5 \
    libqt5virtualkeyboard5 \
    libqt5dbus5 \
    qml-module-qtquick2 \
    qml-module-qtquick-controls2 \
    qml-module-qtquick-layouts \
    qml-module-qtquick-window2 \
    qtvirtualkeyboard-plugin \
    network-manager

apt-get clean
rm -rf /var/lib/apt/lists/*

echo "=== Installing application files ==="
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Binary
mkdir -p /usr/bin
if [ -f "$SCRIPT_DIR/usr/bin/robot-browser" ]; then
    cp "$SCRIPT_DIR/usr/bin/robot-browser" /usr/bin/
    chmod 755 /usr/bin/robot-browser
fi

# Startup script
mkdir -p /usr/lib/robot-browser
cp "$SCRIPT_DIR/usr/lib/robot-browser/robotbrowser.sh" /usr/lib/robot-browser/
chmod 755 /usr/lib/robot-browser/robotbrowser.sh

# Virtual keyboard layouts
if [ -d "$SCRIPT_DIR/usr/share/robot-browser/layouts" ]; then
    mkdir -p /usr/share/robot-browser/layouts
    cp -r "$SCRIPT_DIR/usr/share/robot-browser/layouts/"* /usr/share/robot-browser/layouts/
fi

# Config
mkdir -p /etc/robot-browser
if [ ! -f /etc/robot-browser/browser.config ]; then
    cp "$SCRIPT_DIR/etc/robot-browser/browser.config" /etc/robot-browser/
fi

# Systemd service
cp "$SCRIPT_DIR/usr/lib/systemd/system/robot-browser.service" /usr/lib/systemd/system/ 2>/dev/null || \
    cp "$SCRIPT_DIR/usr/lib/systemd/system/robot-browser.service" /etc/systemd/system/

# Udev rules
cp "$SCRIPT_DIR/etc/udev/rules.d/99-robot-input.rules" /etc/udev/rules.d/

echo "=== Enabling services ==="
systemctl daemon-reload
systemctl enable robot-browser.service
systemctl enable NetworkManager

# Disable desktop environment if installed
systemctl set-default multi-user.target

echo "=== Done ==="
echo "Edit /etc/robot-browser/browser.config to set your URLs and layout."
echo "Check /etc/udev/rules.d/99-robot-input.rules for touchscreen device name."
echo "Reboot to start the kiosk browser."
