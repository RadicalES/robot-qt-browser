# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Kiosk-style browser application for embedded Linux (Raspberry Pi CM4, Pi 5). Displays two web pages (a remote transaction URL and a local web UI) with minimal desktop functions: WiFi and LAN setup, SCADA server status, reboot, system info. Built with Qt 6.4.2, a pure Qt Widgets UI, and QtWebEngine (Chromium 102). Targets Debian 12 (Bookworm) arm64.

Development, release and publishing workflow: [WORKFLOW.md](WORKFLOW.md).

## Project Structure

```
robot-qt-browser/
├── src/                    # Application source
│   ├── *.cpp, *.h          # C++ sources (widgets UI + controllers)
│   ├── qml/                # virtualkeyboard.qml only — the keyboard's InputPanel
│   ├── images/             # Icons
│   ├── CMakeLists.txt      # Build definition
│   └── robot-browser.qrc   # Qt resources
├── layouts/                # Qt Virtual Keyboard layouts (three pages)
├── styles/robot/           # Virtual keyboard style
├── packaging/udev/         # Input device rules shipped by the package
├── debian/                 # Package metadata, service unit, config, changelog
├── docker/                 # arm64 cross-compilation (Dockerfile.cm4-qt6)
├── rootfs-cm4-x11/         # Kiosk session overlay (LXDE-pi-robot, X11)
├── scripts/                # build-deb, bump-version, publish-deb, setup-kiosk
└── docs/                   # KIOSK-CM4, WIFI-ROAMING, qt6-wifi-integration
```

## Build

CMake, not qmake — Debian's Qt 6 ships no aarch64 mkspec, so the qmake
cross-build the 2.x line used cannot work.

```sh
./docker/build-cm4.sh                # arm64 → build-cm4/
./scripts/build-deb.sh arm64|amd64   # → build-deb/
cmake -B build-amd64 src && cmake --build build-amd64 -j$(nproc)   # local
```

Qt 6 modules: Core, Gui, Widgets, Network, WebEngineWidgets, WebSockets, DBus, Quick, QuickWidgets. QtVirtualKeyboard is loaded at runtime as a QML module, not linked.

## Architecture

**Pure Qt Widgets.** `QWebEngineView` is the central widget of a `QMainWindow`, with a bottom `QToolBar` and `QDialog` popups. The virtual keyboard shares the central layout so it pushes the page up rather than covering it. There is no QML UI — `src/qml/virtualkeyboard.qml` is the keyboard's `InputPanel` and nothing else.

```
src/main.cpp
├── QApplication + KioskStyle (RSIP_OnMouseClick — keyboard on first tap)
├── QMainWindow
│   ├── central: [offline banner] [QWebEngineView] [load bar] [keyboard panel]
│   └── bottom QToolBar (76px, 48px icons, #4d4d4d)
│       [Home] [Remote] [Back] ──spacer── [WiFi] [LAN] [SCADA] [Keyboard] [Clock] [Info]
│
├── Controllers (plain QObjects, wired to widget signals)
│   ├── WebPageController  — loadLocal(), loadRemote(), reload(), goBack()
│   ├── NetworkController  — WiFi and LAN via NetworkManager D-Bus
│   ├── SystemController   — reboot(), resetDefaults(), systemInfo()
│   └── WebsockServer      — debug WebSocket server (port 7070)
│
└── Widgets (src/)
    ├── digitalclock.h        — header-only QLabel clock
    ├── scadaindicator.h      — SCADA state from /run/robot, robot head icon
    ├── virtualkeyboardpanel.h— hosts the keyboard's InputPanel in a QQuickWidget
    ├── wifidialog.*          — WiFi scan, connect, forget, signal
    ├── landialog.*           — wired link state and IPv4 config
    ├── ipconfigdialog.*      — shared DHCP/static editor
    ├── infodialog.*          — system info, reset defaults, reboot
    └── rebootdialog.*        — reboot confirmation
```

**Key classes (all in `src/`):**
- `WebPageController` — owns the view, a `WebPage` and a persistent `QWebEngineProfile("robot-browser")` for cookies. Destruction order matters: the page must outlive nothing and the profile must outlive the page, so the destructor is explicit and the view is held in a `QPointer`. Refocuses the view on `loadFinished`.
- `WebPage` — `QWebEnginePage` subclass. Navigation network check, HTTP auth dialog, JS console forwarding to the debug server. Emits `networkUnavailable()` instead of showing a modal; main.cpp shows an inline banner.
- `VirtualKeyboardPanel` — hosts `qrc:/qml/virtualkeyboard.qml` in a `QQuickWidget` with `Qt::NoFocus`. Debian's Qt 6 QtVirtualKeyboard has **no desktop integration compiled in**, so the panel must be hosted by the application. Height follows the QML root; `isShowing()` queries the QML `keyboardActive` property, not widget visibility.
- `KioskStyle` — `QProxyStyle` returning `RSIP_OnMouseClick`, so an input raises the keyboard on the first tap.
- `DialogStyle` — shared dialog sizing (`sheet()`, `widthToScreen()`, `centerOnScreen()`), the colour set, `takeNoFocusExceptFields()` and `closeKeyboard()`.
- `AppConfig` — reads `/etc/robot-browser/browser.config`; CLI arguments override it.
- `ScadaIndicator` — polls `/run/robot` (status, last_ok, station, serverURL), the same files robot-scada-client's tray indicator reads. Needs no D-Bus, no socket and not even the client package.
- `PageFocusGuard` — application event filter that reclaims page focus on tap.
- `UnixSignalNotifier` — SIGINT/SIGTERM to Qt signals for clean shutdown.

**The keyboard's hard constraint:** the input method binds to the *active window*, not the focused widget. After any dialog closes, the main window must `raise()` and `activateWindow()` before the keyboard will work again — see `refocusPage()` in main.cpp. This cost five wrong diagnoses; do not "simplify" it away.

## Configuration

`/etc/robot-browser/browser.config`:

```sh
WB_REMOTE_URL=...     # [Remote] button and the landing page
WB_LOCAL_URL=...      # [Home] button — the local web UI
NETWORK_WIFI=on       # auto | on | off
NETWORK_LAN=off       # auto | on | off
```

Deployments differ: T430 is wired, T440 is WiFi roaming, general terminals leave both on `auto`. The package is standalone — it must not depend on any other Radical software being installed.

## Deployment

Debian 12 arm64 under X11 (Xwayland is fine). QtWebEngine needs a GPU and a compositor, so `linuxfb` is not an option and the BeagleBone Black armhf target was retired at 3.0 — the last BBB build is on the `webkit` branch.

The package ships a systemd unit but does **not** enable it: the kiosk session starts the browser, and an enabled service fights it. Restarting the browser means restarting `lightdm`.

See `rootfs-cm4-x11/`, `scripts/setup-kiosk-cm4.sh` and `docs/KIOSK-CM4.md`. WiFi hardware notes: `docs/WIFI-ROAMING.md`.

## Code Conventions

- C++17, Qt 6 APIs only
- New-style `connect(&obj, &Class::signal, ...)`
- Controllers expose state via Q_PROPERTY with NOTIFY; widgets connect directly
- UI is Qt Widgets only — do not reintroduce a QML UI
- No address bar — two fixed URLs
