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

# --- Qt platform ---
# Auto-detect the display stack. X11 is preferred where both are available:
# Qt 6 refuses the on-screen keyboard on Wayland, and these terminals are
# touch-only. With no display server at all, eglfs drives KMS/DRM directly.
#
# linuxfb is deliberately absent: QtWebEngine needs EGL/GLES and cannot run on
# it, which is why the BeagleBone target was retired.
if [ -n "$DISPLAY" ]; then
    export QT_QPA_PLATFORM=xcb
elif [ -n "$WAYLAND_DISPLAY" ] || [ "$XDG_SESSION_TYPE" = "wayland" ]; then
    export QT_QPA_PLATFORM=wayland
    export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
else
    export QT_QPA_PLATFORM=eglfs
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
exec $DAEMON $WB_REMOTE_URL $WB_LOCAL_URL
