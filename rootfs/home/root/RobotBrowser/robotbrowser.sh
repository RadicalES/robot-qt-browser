#!/bin/sh
# (C) 2017-2025, Radical Electronic Systems
# Author:
# Jan Zwiegers, jan@radicalsystems.co.za
#
# Robot Kiosk Browser startup script for Debian 12

DAEMON=/home/root/RobotBrowser/robot-browser

# defaults
WB_ANGLE=270

# source settings
. /etc/formfactor/appconfig

if [ "$WB_LAYOUT" = "portrait" ]; then
    WB_ANGLE=270
else
    WB_ANGLE=0
fi

# --- Qt platform ---
# linuxfb for BBB (no GPU), eglfs for RPi CM4 (optional)
export QT_QPA_PLATFORM=linuxfb:rotation=$WB_ANGLE

# --- Input devices ---
# Use evdev for touchscreen and keyboard (Debian 12 default, no tslib needed)
if [ -e /dev/input/touchscreen0 ]; then
    export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=/dev/input/touchscreen0:rotate=$WB_ANGLE
    export QT_QPA_GENERIC_PLUGINS=evdevtouch:/dev/input/touchscreen0
fi

if [ -e /dev/input/keyboard0 ]; then
    export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/keyboard0:grab=1
fi

# --- Virtual keyboard ---
export QT_IM_MODULE=qtvirtualkeyboard
export QT_VIRTUALKEYBOARD_LAYOUT_PATH=/home/root/RobotBrowser/layouts

# --- Framebuffer ---
export QT_QPA_FB_DRM=1
export QT_QPA_FB_NO_LIBINPUT=1

# Start Robot Kiosk Browser
# Args: <remote_url> [local_url]
cd /home/root/RobotBrowser
exec $DAEMON $WB_LOAD_URL
