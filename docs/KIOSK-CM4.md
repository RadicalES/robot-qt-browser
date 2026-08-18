# Booting straight into the browser (CM4 / Pi 5)

How to run robot-browser as the whole device: no desktop, no panel, nothing but
the kiosk. Mirrors the T430 rootfs session layout, so these files transfer to the
dedicated rootfs repo unchanged.

## Why X11 and not Wayland

Qt 6 refuses the client-side virtual keyboard on Wayland:

```
qt.qpa.wayland: qtvirtualkeyboard currently is not supported at client-side,
                use QT_IM_MODULE=qtvirtualkeyboard at compositor-side.
```

labwc provides no compositor-side input method, so the on-screen keyboard cannot
work under the labwc session — and the terminal is touch-only, so that is fatal.
X11 has no such restriction. eglfs works too, for a future no-display-server
setup. See issue #6.

## Install

On the device, as root:

```sh
# 1. the browser itself
sudo apt install ./robot-browser_2.1.0-1_arm64.deb

# 2. the kiosk session
sudo ./scripts/setup-kiosk-cm4.sh

# 3. point it at the real URLs
sudo nano /etc/formfactor/app.conf

sudo reboot
```

`setup-kiosk-cm4.sh` installs the X11 session from `rootfs-cm4-x11/`, points
LightDM's autologin at it, and disables the packaged systemd unit (see below).
It never overwrites an existing `/etc/formfactor/app.conf`.

To get the desktop back: `sudo ./scripts/setup-kiosk-cm4.sh --undo`, then reboot.

## Boot sequence

```
lightdm  (autologin-user=robot, autologin-session=LXDE-pi-robot)
  └── /usr/bin/startlxde-pi-robot
        └── lxsession -s LXDE-pi-robot -e LXDE
              └── /etc/xdg/lxsession/LXDE-pi-robot/autostart
                    ├── xset s off / dpms off   (never blank a kiosk)
                    ├── unclutter                (hide pointer — touch only)
                    └── pi-app.sh                (dispatch on START_APP)
                          └── pi-browser.sh      (BROWSER)
                                └── /usr/bin/robot-browser <remote> <local>
```

The `@` prefix on the autostart line means lxsession respawns the app if it
exits, so a crash returns to the kiosk rather than a bare desktop.

## URLs

`/etc/formfactor/app.conf` — same file and variable names as the T430 rootfs:

| Variable | Used for |
|---|---|
| `SERVER_CONFIG_URL` | local web UI — the **[Home]** button |
| `SERVER_TRANSACTION_URL` | remote transaction page — the **[Remote]** button |
| `START_APP` | which app the session launches (`BROWSER`) |

When the terminal is provisioned, robot-scada-client writes the server's
decision to `/etc/formfactor/server.conf`, which `pi-app.sh` and `pi-browser.sh`
source **after** `app.conf` so it takes precedence. Unlike the CHROME app, the
browser takes both URLs and switches between them from its own toolbar.

`pi-browser.sh` waits up to `NETWORK_WAIT` seconds (default 30) for an address
on any non-loopback interface, then starts regardless — an offline or WiFi-only
terminal still shows its UI rather than hanging on a blank screen.

## Runtime packages

Pulled in by the .deb, listed here because two of them fail *silently* when
missing — the keyboard simply never appears:

- `qt6-virtualkeyboard-plugin`, `qml6-module-qtquick-virtualkeyboard`
- `qml6-module-qtqml-workerscript`, `qml6-module-qt-labs-folderlistmodel`

Custom keyboard layouts install to `/usr/share/robot-browser/layouts` and are
picked up via `QT_VIRTUALKEYBOARD_LAYOUT_PATH`, set by `pi-browser.sh`.

## The packaged systemd unit

The .deb ships `robot-browser.service`, which runs the browser **as root on
linuxfb** via `robotbrowser.sh`. That is the BeagleBone launch path and it
fights this session for the display. `setup-kiosk-cm4.sh` disables it; the
session owns the browser's lifecycle here.

## Verifying

```sh
pgrep -a robot-browser                       # should show both URLs
DISPLAY=:0 scrot -o /tmp/shot.png            # screenshot the kiosk
tail -f ~/.cache/lxsession/LXDE-pi-robot/run.log
```
