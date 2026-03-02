#!/bin/sh
# (C) 2017-2026, Radical Electronic Systems
# Robot Kiosk Browser startup script

DAEMON=/usr/bin/robot-browser

# defaults
WB_ANGLE=270

# source settings
. /etc/robot-browser/appconfig

if [ "$WB_LAYOUT" = "portrait" ]; then
    WB_ANGLE=270
else
    WB_ANGLE=0
fi

# --- Qt platform ---
# Auto-detect: use wayland if available, fall back to linuxfb
if [ -n "$WAYLAND_DISPLAY" ] || [ -n "$XDG_SESSION_TYPE" ] && [ "$XDG_SESSION_TYPE" = "wayland" ]; then
    export QT_QPA_PLATFORM=wayland
    export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
elif [ -n "$DISPLAY" ]; then
    export QT_QPA_PLATFORM=xcb
else
    export QT_QPA_PLATFORM=linuxfb:rotation=$WB_ANGLE
    export QT_QPA_FB_DRM=1
    export QT_QPA_FB_NO_LIBINPUT=1
fi

# --- Input devices ---
if [ -e /dev/input/touchscreen0 ]; then
    export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=/dev/input/touchscreen0:rotate=$WB_ANGLE
    export QT_QPA_GENERIC_PLUGINS=evdevtouch:/dev/input/touchscreen0
fi

if [ -e /dev/input/keyboard0 ]; then
    export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/keyboard0:grab=1
fi

# --- Virtual keyboard ---
export QT_IM_MODULE=qtvirtualkeyboard
export QT_VIRTUALKEYBOARD_LAYOUT_PATH=/usr/share/robot-browser/layouts

# Start Robot Kiosk Browser
# Args: <remote_url> [local_url]
exec $DAEMON $WB_LOAD_URL
