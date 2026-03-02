#!/bin/sh
# (C) 2017-2026, Radical Electronic Systems
# Robot Kiosk Browser startup for CM4 (Wayland/labwc session)

DAEMON=/usr/bin/robot-browser

# source settings
. /etc/robot-browser/browser.config

# Qt runs under Wayland compositor (labwc)
export QT_QPA_PLATFORM=wayland
export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
export QT_IM_MODULE=qtvirtualkeyboard
export QT_VIRTUALKEYBOARD_LAYOUT_PATH=/usr/share/robot-browser/layouts

# Start Robot Kiosk Browser
# Args: <remote_url> [local_url]
exec $DAEMON $WB_REMOTE_URL $WB_LOCAL_URL
