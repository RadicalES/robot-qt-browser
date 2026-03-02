#!/bin/sh
# (C) 2017-2025, Radical Electronic Systems
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
    qml-module-qtquick2 \
    qml-module-qtquick-controls2 \
    qml-module-qtquick-layouts \
    qml-module-qtquick-window2 \
    qtvirtualkeyboard-plugin \
    network-manager

apt-get clean
rm -rf /var/lib/apt/lists/*

echo "=== Installing application files ==="
mkdir -p /home/root/RobotBrowser
mkdir -p /etc/formfactor

# Copy binary (caller must place robot-browser in same directory as this script)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [ -f "$SCRIPT_DIR/home/root/RobotBrowser/robot-browser" ]; then
    cp "$SCRIPT_DIR/home/root/RobotBrowser/robot-browser" /home/root/RobotBrowser/
    chmod +x /home/root/RobotBrowser/robot-browser
fi

cp "$SCRIPT_DIR/home/root/RobotBrowser/robotbrowser.sh" /home/root/RobotBrowser/
chmod +x /home/root/RobotBrowser/robotbrowser.sh

# Copy layouts if present
if [ -d "$SCRIPT_DIR/home/root/RobotBrowser/layouts" ]; then
    cp -r "$SCRIPT_DIR/home/root/RobotBrowser/layouts" /home/root/RobotBrowser/
fi

# Config and services
cp "$SCRIPT_DIR/etc/formfactor/appconfig" /etc/formfactor/
cp "$SCRIPT_DIR/etc/systemd/system/browser.service" /etc/systemd/system/
cp "$SCRIPT_DIR/etc/udev/rules.d/99-robot-input.rules" /etc/udev/rules.d/

echo "=== Enabling services ==="
systemctl daemon-reload
systemctl enable browser.service
systemctl enable NetworkManager

# Disable desktop environment if installed
systemctl set-default multi-user.target

echo "=== Done ==="
echo "Edit /etc/formfactor/appconfig to set your URL and layout."
echo "Check /etc/udev/rules.d/99-robot-input.rules for touchscreen device name."
echo "Reboot to start the kiosk browser."
