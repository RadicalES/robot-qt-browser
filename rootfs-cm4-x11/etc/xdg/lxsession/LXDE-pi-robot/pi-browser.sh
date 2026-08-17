#!/bin/bash
#
# pi-browser.sh - launch robot-browser (the BROWSER app), invoked by pi-app.sh.
#
# Opens the server's transactionURL as the remote page and the local web-UI URL
# as the home page. Both come from config files that exist at boot, so the kiosk
# launches immediately without waiting on robot-scada-client to re-contact the
# server.
#
# robot-browser takes both URLs and switches between them from its own toolbar:
#   robot-browser <remote_url> [local_url]
# so [Home] goes to the local web UI and [Remote] to the transaction page.
#
# (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za

DAEMON=/usr/bin/robot-browser

# Local settings (SERVER_CONFIG_URL fallback), then the persisted server config.
. /etc/formfactor/app.conf 2>/dev/null || true
[ -r /etc/formfactor/server.conf ] && . /etc/formfactor/server.conf 2>/dev/null || true

LOCAL_URL="${SERVER_CONFIG_URL:-http://127.0.0.1}"
REMOTE_URL="${SERVER_TRANSACTION_URL:-$LOCAL_URL}"

if [ ! -x "$DAEMON" ]; then
    echo "pi-browser: $DAEMON not installed" >&2
    exit 1
fi

# Wait for an address on any non-loopback interface, but never block the kiosk
# forever: a WiFi-only or offline terminal must still show its UI, and the
# browser reports connectivity problems itself.
wait_for_network() {
    waited=0
    while [ "$waited" -lt "${NETWORK_WAIT:-30}" ]; do
        if ip -4 addr show scope global 2>/dev/null | grep -q "inet "; then
            return 0
        fi
        sleep 2
        waited=$((waited + 2))
    done
    echo "pi-browser: no network after ${NETWORK_WAIT:-30}s - starting anyway" >&2
}

wait_for_network

# The kiosk is touch-only, so the on-screen keyboard is the only way to type.
# QtVirtualKeyboard is hosted inside the browser; it works under X11 and eglfs
# but NOT under Wayland, where Qt refuses the client-side input context.
export QT_QPA_PLATFORM=xcb
export QT_IM_MODULE=qtvirtualkeyboard
[ -d /usr/share/robot-browser/layouts ] && \
    export QT_VIRTUALKEYBOARD_LAYOUT_PATH=/usr/share/robot-browser/layouts

echo "pi-browser: remote=$REMOTE_URL local=$LOCAL_URL"
exec "$DAEMON" "$REMOTE_URL" "$LOCAL_URL"
