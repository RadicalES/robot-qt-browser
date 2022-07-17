#!/bin/sh
# (C) 2017-2022, Radical Electronic Systems
# Author:
# Jan Zwiegers, jan@radicalsystems.co.za

# App itself
DAEMON=/home/root/RobotBrowser/RBrowser
# default startup page
DAEMON_ARGS="http://127.0.0.1"

# default 
#WB_LAYOUT=portrait
WB_ANGLE=270

# source settings
. /etc/formfactor/appconfig

if [ $WB_LAYOUT = "portrait" ]; then
	WB_ANGLE=270
else
	WB_ANGLE=0
fi

res=$(cat /sys/class/drm/card0/*/modes | tr -d '\n')
	
# Check if touchscreen is available
if [ -x /usr/bin/ts_calibrate ]; then

    if [ -e /sys/class/graphics/fb0/modes ]; then
        head -1 /sys/class/graphics/fb0/modes > /sys/class/graphics/fb0/mode
    fi

    # setup touchscreen calibration
    rm -f /etc/pointercal
    if [ ! -f /etc/pointercal-${res} ]; then
        TSLIB_TSDEVICE=/dev/input/touchscreen0 /usr/bin/ts_calibrate
        mv /etc/pointercal /etc/pointercal-${res}
    fi

    ln -sf /etc/pointercal-${res} /etc/pointercal
    export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/keyboard0;grab=1
    export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/touchscreen0
    export QT_QPA_PLATFORM=linuxfb:rotation=$WB_ANGLE
    export TSLIB_ROTATE=$WB_ANGLE
    export QT_IM_MODULE=qtvirtualkeyboard
    export QT_VIRTUALKEYBOARD_LAYOUT_PATH=/home/root/RobotBrowser/layouts
else
    export QT_QPA_GENERIC_PLUGINS=evdevkeyboard:/dev/input/keyboard0
fi

# Run all profiles
. /etc/profile

# Start Robot Kiosk Browser
DAEMON_ARGS="$WB_LOAD_URL"
cd /home/root/RobotBrowser
status = $($DAEMON $DAEMON_ARGS)

return status
