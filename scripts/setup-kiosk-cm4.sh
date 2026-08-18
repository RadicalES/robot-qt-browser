#!/bin/sh
# Set up a CM4 / Pi 5 to boot straight into robot-browser and nothing else.
#
# Installs the X11 kiosk session from rootfs-cm4-x11/ and points LightDM's
# autologin at it. Mirrors the T430 rootfs layout, so these files transfer to
# the dedicated rootfs repo unchanged.
#
# X11 rather than Wayland is deliberate: Qt 6 refuses the client-side virtual
# keyboard on Wayland and labwc has no compositor-side input method, so the
# on-screen keyboard cannot work under the labwc session. See issue #6.
#
# Run on the device as root:
#   sudo ./setup-kiosk-cm4.sh
#
# Leaves the desktop session installed — pick it from LightDM, or run
# --undo to restore it as the autologin session.
#
# (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OVERLAY="${OVERLAY:-$PROJECT_DIR/rootfs-cm4-x11}"

KIOSK_SESSION="LXDE-pi-robot"
DESKTOP_SESSION="LXDE-pi-labwc"
KIOSK_USER="${KIOSK_USER:-robot}"
LIGHTDM_CONF=/etc/lightdm/lightdm.conf

if [ "$(id -u)" != "0" ]; then
    echo "Run as root: sudo $0" >&2
    exit 1
fi

# --- undo -------------------------------------------------------------------
if [ "$1" = "--undo" ]; then
    echo "=== Restoring $DESKTOP_SESSION as the autologin session ==="
    sed -i "s/^autologin-session=.*/autologin-session=$DESKTOP_SESSION/" "$LIGHTDM_CONF"
    grep -E "^autologin-session=" "$LIGHTDM_CONF"
    echo "Reboot to apply."
    exit 0
fi

if [ ! -d "$OVERLAY" ]; then
    echo "Overlay not found: $OVERLAY" >&2
    echo "Copy rootfs-cm4-x11/ to the device, or set OVERLAY=<path>." >&2
    exit 1
fi

echo "=== Installing kiosk session packages ==="
# lxsession/openbox provide the session; unclutter hides the pointer on a
# touch-only terminal. robot-browser itself pulls in its own Qt 6 runtime.
apt-get update -qq
apt-get install -y --no-install-recommends \
    lightdm lxsession openbox xserver-xorg xinit x11-xserver-utils unclutter

if ! command -v robot-browser >/dev/null 2>&1; then
    echo "WARNING: robot-browser is not installed."
    echo "         Install the .deb first: apt install ./robot-browser_*_arm64.deb"
fi

echo "=== Installing session overlay ==="
# Device settings are never clobbered: app.conf is seeded only when absent.
for f in usr/share/xsessions/LXDE-pi-robot.desktop \
         usr/bin/startlxde-pi-robot \
         etc/xdg/lxsession/LXDE-pi-robot/autostart \
         etc/xdg/lxsession/LXDE-pi-robot/pi-app.sh \
         etc/xdg/lxsession/LXDE-pi-robot/pi-browser.sh; do
    install -D -m 0644 "$OVERLAY/$f" "/$f"
    echo "  /$f"
done
chmod 0755 /usr/bin/startlxde-pi-robot \
           /etc/xdg/lxsession/LXDE-pi-robot/pi-app.sh \
           /etc/xdg/lxsession/LXDE-pi-robot/pi-browser.sh

# WiFi configuration from the kiosk needs polkit permission for the writes.
# Scanning and reading state do not, so this only matters once an operator
# tries to join a network.
install -D -m 0644 "$OVERLAY/etc/polkit-1/rules.d/50-robot-network.rules" \
    /etc/polkit-1/rules.d/50-robot-network.rules
echo "  /etc/polkit-1/rules.d/50-robot-network.rules"

if [ ! -f /etc/formfactor/app.conf ]; then
    install -D -m 0644 "$OVERLAY/etc/formfactor/app.conf" /etc/formfactor/app.conf
    echo "  /etc/formfactor/app.conf (seeded)"
else
    echo "  /etc/formfactor/app.conf (kept — existing device settings)"
fi

echo "=== Pointing LightDM autologin at the kiosk session ==="
if grep -qE "^autologin-session=" "$LIGHTDM_CONF"; then
    sed -i "s/^autologin-session=.*/autologin-session=$KIOSK_SESSION/" "$LIGHTDM_CONF"
else
    printf '\n[Seat:*]\nautologin-user=%s\nautologin-session=%s\n' \
        "$KIOSK_USER" "$KIOSK_SESSION" >> "$LIGHTDM_CONF"
fi
if ! grep -qE "^autologin-user=" "$LIGHTDM_CONF"; then
    sed -i "/^autologin-session=/i autologin-user=$KIOSK_USER" "$LIGHTDM_CONF"
fi
grep -E "^autologin-(user|session)=" "$LIGHTDM_CONF" | sed 's/^/  /'

# The packaged systemd unit runs the browser as root on linuxfb. That is the
# BeagleBone launch path and it fights this session for the display.
if systemctl is-enabled robot-browser >/dev/null 2>&1; then
    echo "=== Disabling robot-browser.service (the session launches it) ==="
    systemctl disable --now robot-browser || true
fi

echo
echo "=== Done ==="
echo "Reboot to come up in the browser:  sudo reboot"
echo "To get the desktop back:           sudo $0 --undo"
