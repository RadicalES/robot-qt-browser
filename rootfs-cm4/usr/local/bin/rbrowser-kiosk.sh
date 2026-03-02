#!/bin/sh
# (C) 2017-2025, Radical Electronic Systems
# Robot Kiosk Browser startup for CM4 (Wayland/labwc session)

DAEMON=/home/root/RobotBrowser/robot-browser

# source settings
. /etc/formfactor/appconfig

# Qt runs under Wayland compositor (labwc)
export QT_QPA_PLATFORM=wayland
export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
export QT_IM_MODULE=qtvirtualkeyboard
export QT_VIRTUALKEYBOARD_LAYOUT_PATH=/home/root/RobotBrowser/layouts

# Start Robot Kiosk Browser
# Args: <remote_url> [local_url]
cd /home/root/RobotBrowser
exec $DAEMON $WB_LOAD_URL
