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
./scripts/build-deb.sh arm64       # → build-deb/robot-browser_<version>-1_arm64.deb
```

Local amd64 build for UI work:

```sh
cmake -B build-amd64 src -DCMAKE_BUILD_TYPE=Release && cmake --build build-amd64 -j$(nproc)
```

See [WORKFLOW.md](WORKFLOW.md) for the full development, release and publishing
workflow.

## Running

```sh
robot-browser [--config=PATH] [--wifi=auto|on|off] [--lan=...] [remote_url] [local_url]
```

Settings come from `/etc/robot-browser/browser.config`, and anything on the
command line overrides the file. Both URLs default to `http://127.0.0.1`.

## Installing

Published to the RadicalES package repository, so a terminal installs it like
any other package. Add the repository once:

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
