# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Kiosk-style browser application for embedded Linux (BeagleBone Black, Raspberry Pi CM4). Displays two web pages (a local URL and a remote transaction URL) with minimal desktop functions: WiFi status, network setup, reboot, system info. Built with Qt 5.15 LTS, QML UI shell, and QtWebKit 5.212 for web rendering. Targets Debian 12 (Bookworm).

## Project Structure

```
robot-qt-browser/
├── src/                    # Application source code
│   ├── *.cpp, *.h          # C++ sources and headers
│   ├── qml/                # QML UI files
│   ├── images/             # Icons and images
│   ├── RBrowser.pro        # qmake project file
│   └── RBrowser.qrc        # Qt resource file
├── docker/                 # Cross-compilation Docker setup
│   ├── Dockerfile.bbb      # BBB armhf build container
│   ├── Dockerfile.cm4      # CM4 arm64 build container
│   ├── build-bbb.sh        # BBB build script
│   └── build-cm4.sh        # CM4 build script
├── rootfs/                 # BBB deployment overlay (linuxfb, systemd service)
├── rootfs-cm4/             # CM4 deployment overlay (Wayland/labwc session)
├── layouts/                # Qt Virtual Keyboard custom layouts
└── docs/                   # Documentation
    ├── DEPLOYMENT.md       # Platform deployment guide
    └── WIFI-ROAMING.md     # WiFi roaming configuration
```

## Build Commands

```bash
# Cross-compile for BeagleBone Black (armhf)
./docker/build-bbb.sh

# Cross-compile for Raspberry Pi CM4 (arm64)
./docker/build-cm4.sh
```

Output binaries land in `build-bbb/` and `build-cm4/` (gitignored).

For local development (if Qt 5.15 + WebKit are installed):
```bash
cd src && qmake && make
```

## Architecture

**Hybrid layout:** QWebView (widget) underneath a transparent QQuickWidget overlay for the QML shell.

```
src/main.cpp
├── QApplication + QWebSettings global config
├── QStackedLayout (StackAll mode)
│   ├── QQuickWidget (transparent overlay) ← src/qml/main.qml
│   └── QWebView (QtWebKit rendering)     ← WebPageController
│
├── C++ Controllers (registered as QML context properties)
│   ├── WebPageController  — loadLocal(), loadRemote(), reload(), goBack()
│   ├── WpaController      — signalLevel, connected, ssid, restartWifi()
│   ├── SystemController   — reboot(), resetDefaults(), systemInfo()
│   └── WebsockServer      — debug WebSocket server (port 7070)
│
└── QML UI (src/qml/)
    ├── main.qml         — root overlay with bottom bar, popups, virtual keyboard
    ├── BottomBar.qml    — [Home] [Remote] [Back] | WiFi icon | Clock | [Info]
    ├── WifiPopup.qml    — WiFi restart confirmation
    ├── RebootPopup.qml  — Reboot confirmation
    └── InfoPopup.qml    — System info + reset defaults + reboot
```

**Key C++ classes (all in `src/`):**
- `WebPageController` — wraps QWebView + WebPage + CookieJar. Exposes load/reload/back as Q_INVOKABLE, URL and loading state as Q_PROPERTY.
- `WebPage` — simplified QWebPage subclass. Navigation network check, proxy, HTTP auth dialog, JS console/alert forwarding to debug server.
- `WpaController` — **stubbed**, pending NetworkManager D-Bus implementation. Will poll WiFi signal strength and provide restartWifi().
- `SystemController` — reboot, factory reset, system info string.
- `TestBrowserCookieJar` (`cookiejar.h/.cpp`) — disk-persisted cookies, 10-second debounced writes.
- `WebsockServer` — WebSocket server for remote debug, JS console and alert broadcast.
- `UnixSignalNotifier` — singleton converting SIGINT/SIGTERM to Qt signals for systemd shutdown.

## Qt Modules

core, gui, widgets, network, quickwidgets, quickcontrols2, virtualkeyboard, websockets + QtWebKit 5.212 (external)

Enable `dbus` module when implementing NetworkManager WiFi support.

## CLI Arguments

```
RBrowser <remote_url> [local_url]
```

Defaults to `http://127.0.0.1` for both if not provided.

## Resources (src/RBrowser.qrc)

Icons aliased under `qrc:/images/` (home, store, back, info, wifi-off, wifi-0 through wifi-4). QML files under `qrc:/qml/`.

## Deployment

- **BBB:** Debian 12, linuxfb (no display server), systemd service. See `rootfs/` and `docs/DEPLOYMENT.md`.
- **CM4:** Debian 12, Wayland via labwc + LightDM. Three session modes: desktop, Chrome kiosk, RBrowser kiosk. See `rootfs-cm4/` and `docs/DEPLOYMENT.md`.
- **WiFi:** Ezurio ST60-2230C (NXP 88W8997) over SDIO on both platforms. See `docs/WIFI-ROAMING.md`.

## Pending Work

- `WpaController`: implement NetworkManager D-Bus polling for WiFi signal strength, connection status, SSID, and restart
- Virtual keyboard layout path may need updating for Debian 12 Qt packages

## Code Conventions

- C++14 standard
- Qt signal/slot connections (prefer new-style `connect(&obj, &Class::signal, ...)`)
- C++ controllers expose state to QML via Q_PROPERTY with NOTIFY signals
- QML actions call C++ via Q_INVOKABLE methods
- No address bar — kiosk mode with two fixed URLs only
