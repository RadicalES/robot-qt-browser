#!/bin/sh
# (C) 2017-2026, Radical Electronic Systems
# Robot Kiosk Browser startup script

DAEMON=/usr/bin/robot-browser

# defaults
WB_ANGLE=270

# source settings
. /etc/robot-browser/browser.config

if [ "$WB_LAYOUT" = "portrait" ]; then
    WB_ANGLE=270
else
    WB_ANGLE=0
fi

# --- Runtime dir ---
export XDG_RUNTIME_DIR=/tmp/runtime-root
mkdir -p "$XDG_RUNTIME_DIR"

# --- Qt platform ---
# Auto-detect: wayland > xcb > linuxfb
if [ -n "$WAYLAND_DISPLAY" ] || [ -n "$XDG_SESSION_TYPE" ] && [ "$XDG_SESSION_TYPE" = "wayland" ]; then
    export QT_QPA_PLATFORM=wayland
    export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
elif [ -n "$DISPLAY" ]; then
    export QT_QPA_PLATFORM=xcb
else
    # Unbind fbcon so Qt can paint on the framebuffer
    echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null || true
    dd if=/dev/zero of=/dev/fb0 bs=1M 2>/dev/null || true
    export QT_QPA_PLATFORM="linuxfb:fb=/dev/fb0:rotation=$WB_ANGLE"
    export QT_QPA_FB_FORCE_FULLUPDATE=1
fi

# --- Input devices ---
# Qt linuxfb needs explicit evdev plugins for input
PLUGINS=""
if [ -e /dev/input/touchscreen0 ]; then
    export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=/dev/input/touchscreen0:rotate=$WB_ANGLE
    PLUGINS="evdevtouch:/dev/input/touchscreen0"
fi

if [ -e /dev/input/keyboard0 ]; then
    export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/keyboard0:grab=1
fi

# Auto-detect mouse from evdev event devices (grab=1 to prevent duplicate events)
for ev in /dev/input/by-id/*-event-mouse; do
    [ -e "$ev" ] && PLUGINS="${PLUGINS:+$PLUGINS,}evdevmouse:$ev:grab=1"
done
PLUGINS="${PLUGINS:+$PLUGINS,}evdevkeyboard"
export QT_QPA_GENERIC_PLUGINS="$PLUGINS"

# Disable libinput auto-detection to prevent duplicate input devices
export QT_QPA_FB_NO_LIBINPUT=1

# --- Virtual keyboard ---
export QT_IM_MODULE=qtvirtualkeyboard
export QT_VIRTUALKEYBOARD_LAYOUT_PATH=/usr/share/robot-browser/layouts

# Start Robot Kiosk Browser
# Args: <remote_url> [local_url]
exec $DAEMON $WB_REMOTE_URL $WB_LOCAL_URL
