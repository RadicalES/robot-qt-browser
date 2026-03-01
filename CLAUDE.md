# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Kiosk-style browser application for the RadicalES Robot-T410 embedded Linux platform. Built with Qt 5 and QtWebKit 5.212 (custom build). Includes WiFi management via wpa_supplicant, a virtual keyboard, WebSocket debug server, and touchscreen support.

## Build Commands

```bash
qmake RBrowser.pro
make
```

The .pro file expects QtWebKit 5.212 headers at `/home/janz/data/yocto/git/webkit` — adjust INCLUDEPATH/LIBS in `RBrowser.pro` if building on a different machine.

## Architecture

**Entry point:** `qttestbrowser.cpp` — creates `LauncherApplication`, starts a `WebsockServer` (port 7070), and opens a `LauncherWindow`.

**Class hierarchy:**
- `LauncherApplication` (QApplication) — parses CLI args, applies WebKit defaults
- `LauncherWindow` extends `MainWindow` (QMainWindow) — main browser window with toolbar, address bar, status bar
- `WebPage` (QWebPage) — manages navigation, auth, JS console, user agents
- View layer: `WebViewTraditional` (QWebView) or `WebViewGraphicsBased` (QGraphicsView) with `GraphicsWebView`

**Key subsystems:**
- **WiFi/WPA:** `mainwindow.cpp` integrates with `wpa_supplicant/wpa_ctrl.c` via Unix socket. Polls signal strength on a 1-second QTimer. WiFi status icons: `wifi-*.png`.
- **WebSocket debug server:** `websockserver.{h,cpp}` — listens on port 8001, broadcasts debug messages to connected clients.
- **Virtual keyboard:** `keyboardwidget.{h,cpp}` + `keyboard.qml` with layouts in `layouts/` directory.
- **Unix signals:** `unixsignalnotifier.{h,cpp}` — singleton that converts SIGINT/SIGTERM to Qt signals for clean systemd shutdown.
- **Cookie persistence:** `cookiejar.{h,cpp}` — disk-backed cookie storage.
- **URL automation:** `urlloader.{h,cpp}` — batch loads URLs from file (robotized mode via `-r` flag).

**All source files live in the project root** (no src/ subdirectory).

## Qt Modules Used

core, gui, widgets, network, opengl, virtualkeyboard, quickwidgets, websockets, webkit, webkitwidgets

## Deployment

Target runs on Linux framebuffer (`QT_QPA_PLATFORM=linuxfb`). Startup script: `rootfs/home/root/RobotBrowser/robotbrowser.sh`. Systemd unit: `rootfs/etc/systemd/system/browser.service`. App config (default URL, layout): `rootfs/etc/formfactor/appconfig`.

## CLI Arguments

`-graphicsbased`, `-maximize`, `-show-fps`, `-robot-timeout <sec>`, `-inspector-url <url>`, `-local-storage-enabled`, `-no-disk-cookies`, `-r <urlfile>` — see `qttestbrowser.cpp` for full list.

## Code Conventions

- C++11 standard
- Qt signal/slot connections (both old-style SIGNAL/SLOT macros and direct connections)
- `Qt::WA_DeleteOnClose` for window lifecycle management
- QTimer-based periodic tasks (WPA polling, FPS measurement)
- QPointer for preventing dangling pointer access
