#!/bin/bash
#
# pi-app.sh - kiosk session app dispatcher.
#
# Same contract as the T430 rootfs dispatcher so these files drop into that
# layout unchanged: the app comes from the persisted server decision in
# /etc/formfactor/server.conf (written by robot-scada-client) when the terminal
# is provisioned and enabled, otherwise from the local START_APP in app.conf.
#
# This repo only ships the BROWSER app. CHROME and ROBOT-APP live in the T430
# rootfs repo; if either is selected here, say so rather than failing silently
# into a blank screen.
#
# (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za

DIR=/etc/xdg/lxsession/LXDE-pi-robot

. /etc/formfactor/app.conf 2>/dev/null || true
[ -r /etc/formfactor/server.conf ] && . /etc/formfactor/server.conf 2>/dev/null || true

if [ "$SERVER_STATUS" = "ENABLED" ] && [ -n "$SERVER_APP_TYPE" ]; then
    APP="$SERVER_APP_TYPE"
else
    APP="${START_APP:-BROWSER}"
fi
echo "pi-app: launching '$APP'"

case "$APP" in
    BROWSER)   exec "$DIR/pi-browser.sh" ;;
    CHROME|ROBOT-APP)
        echo "pi-app: '$APP' is not shipped in this image - launching BROWSER"
        exec "$DIR/pi-browser.sh" ;;
    DESKTOP)   echo "pi-app: DESKTOP requested in kiosk session - nothing to launch" ;;
    *)         echo "pi-app: unknown app '$APP' - launching BROWSER"
               exec "$DIR/pi-browser.sh" ;;
esac
