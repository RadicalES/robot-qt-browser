# robot-qt-browser

Kiosk browser for RadicalES robot terminals — Raspberry Pi CM4 and Pi 5.

## Overview

Two-URL kiosk browser with WiFi and LAN setup, SCADA server status, system
info and an on-screen keyboard. Built with:

- **Qt 6.4.2** (C++17) on Debian 12 (Bookworm)
- **QtWebEngine** (Chromium 102) — Debian's `qt6-webengine-dev`
- **Qt Widgets** UI — a bottom toolbar and dialogs around a `QWebEngineView`
- **Qt Virtual Keyboard**, hosted in-process (Debian's Qt 6 build has no
  desktop integration)

The terminal shows two fixed pages and no address bar: a remote transaction URL
(the landing page) and a local web UI. Everything else — network setup, system
info, reboot — is in the toolbar.

## Building

```sh
./docker/build-cm4.sh              # arm64 cross-build → build-cm4/robot-browser
./scripts/build-deb.sh arm64       # → build-deb/robot-browser_<version>-1~bookworm_arm64.deb
```

Desktop targets each build inside their own distro, because each ships a
different Qt 6 and QtWebEngine and a binary built against one does not run on
another. Qt 6.4 is the floor, which is why Ubuntu 22.04 (Qt 6.2) is not a
target:

```sh
./docker/build-desktop.sh bookworm|trixie|ubuntu-24.04|ubuntu-26.04
./docker/build-desktop.sh all
./scripts/build-deb.sh trixie      # → robot-browser_<version>-1~trixie_amd64.deb
```

The package version carries the suite it was built for, so apt offers each
machine the build made against its own Qt.

To run one on this machine — installed from its own `.deb`, in its own distro,
with a window on your desktop:

```sh
./docker/run-desktop.sh trixie --profile=t440 https://example.com http://localhost:3000
./docker/run-desktop.sh ubuntu-26.04 --check    # install only: do the Depends resolve?
```

Local amd64 build for UI work:

```sh
cmake -B build-amd64 src -DCMAKE_BUILD_TYPE=Release && cmake --build build-amd64 -j$(nproc)
```

See [WORKFLOW.md](WORKFLOW.md) for the full development, release and publishing
workflow.

## Running

```sh
robot-browser [--profile=kiosk|t430|t440|desktop] [--config=PATH]
              [--wifi=auto|on|off] [--lan=...]
              [--windowed[=WxH]] [--no-toolbar] [remote_url] [local_url]
```

Settings come from `/etc/robot-browser/browser.config`, and anything on the
command line overrides the file. Both URLs default to `http://127.0.0.1`.

### Profiles

A profile says how this instance is meant to run — geometry, which controls
exist, and what is left out because it could not work:

| Profile | Runs as | Notes |
|---------|---------|-------|
| `kiosk` (default) | The terminal | Fullscreen on the panel, toolbar, on-screen keyboard |
| `t430` | A Robot-T430, on a PC | 800×480 window, LAN control, no reboot |
| `t440` | A Robot-T440, on a PC | 800×1280 window, WiFi control, no reboot |
| `desktop` | An application on a PC | Resizable window, no on-screen keyboard, no device controls |

The device profiles exist so somebody writing a webapp for a terminal can see
it at the terminal's exact viewport without having a terminal — see
[docs/WEBAPP-DEVELOPERS.md](docs/WEBAPP-DEVELOPERS.md).

On a PC the package installs a menu entry (**Robot Browser**) for every user on
the machine, with the two device profiles as right-click actions on it.

## Installing

Published to the [RadicalES package repository](https://radicales.net/packages/),
so a terminal installs it like any other package. Add the repository once:

```sh
# Signing key (binary form, for apt's signed-by)
sudo install -d -m 0755 /etc/apt/keyrings
sudo curl -fsSL https://packages.radicales.net/keys/public/radical-debian-binary.gpg \
    -o /etc/apt/keyrings/radical-systems.gpg
sudo chmod 0644 /etc/apt/keyrings/radical-systems.gpg

# Repository — bookworm only
echo "deb [signed-by=/etc/apt/keyrings/radical-systems.gpg] https://packages.radicales.net/debian bookworm main" \
    | sudo tee /etc/apt/sources.list.d/radical-systems.list
```

Then install as usual:

```sh
sudo apt-get update && sudo apt-get install robot-browser
```

Verify the key before trusting it — the fingerprint is
`E455 6EB6 7025 FB34 4051  ED4B 66B8 F7A3 8EB7 2B1C`:

```sh
curl -fsSL https://packages.radicales.net/keys/public/radical-debian.gpg | gpg --with-fingerprint -
```

Note the repository metadata is cached at the edge, so a version published in
the last few hours may not be offered yet.

Setup instructions for other distributions, and the current package list, are at
<https://radicales.net/packages/>.

The package is self-contained: it configures itself from its own config file
and needs no other Radical software present. A provisioning layer, where there
is one, passes the URLs it holds as arguments.

## Architecture

`QWebEngineView` fills the window, with a `QToolBar` along the bottom (Home,
Remote, Back, WiFi, LAN, SCADA status, keyboard toggle, clock, Info) and
`QDialog` popups for settings. The virtual keyboard shares the central layout,
so it pushes the page up rather than covering it.

C++ controllers — `WebPageController`, `NetworkController`, `SystemController`
— expose state via `Q_PROPERTY` with NOTIFY signals, and the widgets connect to
those signals directly. There is no QML UI: the only QML left is the virtual
keyboard's own `InputPanel` and its layouts.

## Target Platforms

- Raspberry Pi CM4 and Pi 5, Debian 12 (Bookworm) arm64
- X11 (Xwayland is fine) — QtWebEngine needs a GPU and a compositor

The BeagleBone Black armhf target was retired at 3.0: QtWebEngine cannot run on
`linuxfb`. The last BBB build is on the `webkit` branch.
