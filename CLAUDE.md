# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Kiosk-style browser application for embedded Linux (BeagleBone Black, Raspberry Pi CM4). Displays two web pages (a local URL and a remote transaction URL) with minimal desktop functions: WiFi status, network setup, reboot, system info. Built with Qt 5.15 LTS, QML UI shell, and QtWebKit 5.212 for web rendering. Targets Debian 12 (Bookworm).

## Build Commands

```bash
qmake RBrowser.pro
make
```

The .pro file expects QtWebKit 5.212 headers at `/home/janz/data/yocto/git/webkit` — adjust INCLUDEPATH/LIBS in `RBrowser.pro` if building on a different machine.

## Architecture

**Hybrid layout:** QWebView (widget) underneath a transparent QQuickWidget overlay for the QML shell.

```
main.cpp
├── QApplication + QWebSettings global config
├── QStackedLayout (StackAll mode)
│   ├── QQuickWidget (transparent overlay) ← qml/main.qml
│   └── QWebView (QtWebKit rendering)     ← WebPageController
│
├── C++ Controllers (registered as QML context properties)
│   ├── WebPageController  — loadLocal(), loadRemote(), reload(), goBack()
│   ├── WpaController      — signalLevel, connected, ssid, restartWifi()
│   ├── SystemController   — reboot(), resetDefaults(), systemInfo()
│   └── WebsockServer      — debug WebSocket server (port 7070)
│
└── QML UI (qml/)
    ├── main.qml         — root overlay with bottom bar, popups, virtual keyboard
    ├── BottomBar.qml    — [Home] [Remote] [Back] | WiFi icon | Clock | [Info]
    ├── WifiPopup.qml    — WiFi restart confirmation
    ├── RebootPopup.qml  — Reboot confirmation
    └── InfoPopup.qml    — System info + reset defaults + reboot
```

**Entry point:** `main.cpp` — sets up QApplication, global QWebSettings, registers C++ controllers to QML context, creates hybrid stacked layout, shows fullscreen.

**Key C++ classes:**
- `WebPageController` (`webpagecontroller.h/.cpp`) — wraps QWebView + WebPage + CookieJar. Exposes load/reload/back as Q_INVOKABLE, URL and loading state as Q_PROPERTY.
- `WebPage` (`webpage.h/.cpp`) — simplified QWebPage subclass. Handles navigation request with network check, proxy from `http_proxy` env, HTTP auth dialog, JS console/alert forwarding to debug server.
- `WpaController` (`wpacontroller.h/.cpp`) — **stubbed**, pending NetworkManager D-Bus implementation. Will poll WiFi signal strength and provide restartWifi().
- `SystemController` (`systemcontroller.h/.cpp`) — reboot, factory reset, system info string.
- `TestBrowserCookieJar` (`cookiejar.h/.cpp`) — disk-persisted cookies at `~/.cache/RobotBrowser/cookieJar`, 10-second debounced writes.
- `WebsockServer` (`websockserver.h/.cpp`) — WebSocket server for remote debug, JS console and alert messages broadcast to connected clients.
- `UnixSignalNotifier` (`unixsignalnotifier.h/.cpp`) — singleton converting SIGINT/SIGTERM to Qt signals for systemd shutdown.

**All C++ source files live in the project root. QML files in `qml/`.**

## Qt Modules

core, gui, widgets, network, quickwidgets, quickcontrols2, virtualkeyboard, websockets + QtWebKit 5.212 (external)

Enable `dbus` module when implementing NetworkManager WiFi support.

## CLI Arguments

```
RBrowser <remote_url> [local_url]
```

Defaults to `http://127.0.0.1` for both if not provided.

## Resources (RBrowser.qrc)

Icons aliased under `qrc:/images/` (home, store, back, info, wifi-off, wifi-0 through wifi-4). QML files under `qrc:/qml/`.

## Deployment

Target: Debian 12 on BBB/RPi CM4, Linux framebuffer (`QT_QPA_PLATFORM=linuxfb`). Startup script: `rootfs/home/root/RobotBrowser/robotbrowser.sh`. Systemd unit: `rootfs/etc/systemd/system/browser.service`. App config: `rootfs/etc/formfactor/appconfig` (WB_LOAD_URL, WB_LAYOUT).

## Pending Work

- `WpaController`: implement NetworkManager D-Bus polling for WiFi signal strength, connection status, SSID, and restart
- Virtual keyboard layout path may need updating for Debian 12 Qt packages

## Code Conventions

- C++14 standard
- Qt signal/slot connections (prefer new-style `connect(&obj, &Class::signal, ...)`)
- C++ controllers expose state to QML via Q_PROPERTY with NOTIFY signals
- QML actions call C++ via Q_INVOKABLE methods
- No address bar — kiosk mode with two fixed URLs only
