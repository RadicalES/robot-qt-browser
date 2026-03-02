# robot-qt-browser

Kiosk browser and minimal desktop for the RadicalES Robot-T410, BeagleBone Black, and Raspberry Pi CM4.

## Overview

Two-URL kiosk browser with WiFi status, network setup, reboot, and system info. Built with:

- **Qt 5.15 LTS** (C++14) on Debian 12 (Bookworm)
- **QtWebKit 5.212** — custom build of the last open-source release ([source](https://github.com/qt/qtwebkit))
- **QML UI shell** — bottom toolbar, popups, virtual keyboard overlay on top of QWebView

## Building

```bash
cd src && qmake && make
```

Cross-compile for target devices:

```bash
./docker/build-bbb.sh    # BeagleBone Black (armhf) → build-bbb/robot-browser
./docker/build-cm4.sh    # Raspberry Pi CM4 (arm64) → build-cm4/robot-browser
```

See [WORKFLOW.md](WORKFLOW.md) for full development workflow, branching strategy, and deployment steps.

## Running

```bash
./robot-browser <remote_url> [local_url]
```

Both default to `http://127.0.0.1` if not provided. On the target device, the startup script `rootfs/home/root/RobotBrowser/robotbrowser.sh` handles touchscreen calibration, screen rotation, and environment setup.

## Architecture

Hybrid widget + QML layout: QWebView renders web content underneath a transparent QQuickWidget overlay that provides the bottom toolbar (Home, Remote, Back, WiFi, Clock, Info buttons), confirmation popups, and the Qt Virtual Keyboard.

C++ controllers (`WebPageController`, `NetworkController`, `SystemController`) are registered as QML context properties and expose state via `Q_PROPERTY` / actions via `Q_INVOKABLE`.

## Browser Compatibility

QtWebKit 5.212 (Safari 10 era) has limited JS/CSS support. robot-browser injects polyfills to extend compatibility:

- **JS polyfills** (before page scripts): fetch, URLSearchParams, Object.values/entries, Array.flat, NodeList.forEach
- **CSS polyfills** (after DOM ready): CSS Variables (css-vars-ponyfill), position: sticky (stickyfill), smooth scrolling
- **htmx**: fully compatible
- **Alpine.js v2**: compatible (v3 requires Proxy — not available)

See [docs/BROWSER-COMPATIBILITY.md](docs/BROWSER-COMPATIBILITY.md) for full test results. Run `docs/feature-test.html` in robot-browser to verify on a target device.

## Target Platforms

- BeagleBone Black (ARM Cortex-A8, 512 MB RAM)
- Raspberry Pi Compute Module 4
- Debian 12 (Bookworm), Linux framebuffer (`linuxfb`)
