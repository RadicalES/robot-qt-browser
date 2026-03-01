# robot-qt-browser

Kiosk browser and minimal desktop for the RadicalES Robot-T410, BeagleBone Black, and Raspberry Pi CM4.

## Overview

Two-URL kiosk browser with WiFi status, network setup, reboot, and system info. Built with:

- **Qt 5.15 LTS** (C++14) on Debian 12 (Bookworm)
- **QtWebKit 5.212** — custom build of the last open-source release ([source](https://github.com/qt/qtwebkit))
- **QML UI shell** — bottom toolbar, popups, virtual keyboard overlay on top of QWebView

## Building

```bash
qmake RBrowser.pro
make
```

Requires Qt 5.15 with modules: core, gui, widgets, network, quickwidgets, quickcontrols2, virtualkeyboard, websockets. QtWebKit 5.212 headers/libs path configured in `RBrowser.pro`.

## Running

```bash
./RBrowser <remote_url> [local_url]
```

Both default to `http://127.0.0.1` if not provided. On the target device, the startup script `rootfs/home/root/RobotBrowser/robotbrowser.sh` handles touchscreen calibration, screen rotation, and environment setup.

## Architecture

Hybrid widget + QML layout: QWebView renders web content underneath a transparent QQuickWidget overlay that provides the bottom toolbar (Home, Remote, Back, WiFi, Clock, Info buttons), confirmation popups, and the Qt Virtual Keyboard.

C++ controllers (`WebPageController`, `WpaController`, `SystemController`) are registered as QML context properties and expose state via `Q_PROPERTY` / actions via `Q_INVOKABLE`.

## Target Platforms

- BeagleBone Black (ARM Cortex-A8, 512 MB RAM)
- Raspberry Pi Compute Module 4
- Debian 12 (Bookworm), Linux framebuffer (`linuxfb`)
