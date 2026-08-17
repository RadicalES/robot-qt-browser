# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Kiosk-style browser application for embedded Linux (BeagleBone Black, Raspberry Pi CM4). Displays two web pages (a local URL and a remote transaction URL) with minimal desktop functions: WiFi status, network setup, reboot, system info. Built with Qt 5.15 LTS, a pure Qt Widgets UI shell, and QtWebKit 5.212 for web rendering. Targets Debian 12 (Bookworm).

## Project Structure

```
robot-qt-browser/
├── src/                    # Application source code
│   ├── *.cpp, *.h          # C++ sources and headers (widgets UI + controllers)
│   ├── js/                 # JS/CSS polyfills injected into pages
│   ├── qml/                # Legacy QML UI — no longer built (see Architecture)
│   ├── images/             # Icons and images
│   ├── robot-browser.pro        # qmake project file
│   └── robot-browser.qrc        # Qt resource file
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

## Version & Release

Single source of truth: `VERSION` file in project root. Injected at compile time via `APP_VERSION` define in `robot-browser.pro`.

```bash
# Bump version
./scripts/bump-version.sh patch          # 2.1.0 → 2.1.1
./scripts/bump-version.sh minor          # 2.1.0 → 2.2.0
./scripts/bump-version.sh 3.0.0          # Set explicit version

# Create release (drafts notes into RELEASE.md, commits, tags)
./release.sh                             # Interactive
./release.sh --dry-run                   # Preview only

# Push release (branch + tag to origin, triggers GitHub Actions)
./push-release.sh
```

Tag format: `{branch}-v{version}` (e.g. `dev-v2.1.0`). GitHub Actions auto-creates a release on tag push; marked pre-release if not on `master`.

Branch workflow: `dev` → `beta` (testing) → `master` (production).

## Build Commands

```bash
# Cross-compile for BeagleBone Black (armhf)
./docker/build-bbb.sh

# Cross-compile for Raspberry Pi CM4 (arm64)
./docker/build-cm4.sh

# Build Debian package
./scripts/build-deb.sh arm64|armhf|amd64
```

Output binaries land in `build-bbb/`, `build-cm4/`, `build-amd64/` (gitignored).

For local development (if Qt 5.15 + WebKit are installed):
```bash
cd build-amd64 && qmake ../src/robot-browser.pro && make
```

## Architecture

**Pure Qt Widgets layout:** `KioskWebView` is the `QMainWindow` central widget, with a bottom `QToolBar` and `QDialog` popups. There is no QML — the previous transparent `QQuickWidget` overlay was removed because event forwarding into the web view was broken on linuxfb (events synthesized via `sendEvent` arrive non-spontaneous). `src/qml/` is still in the repo but is not referenced by `robot-browser.pro` or `.qrc`.

```
src/main.cpp
├── QApplication + QWebSettings global config
├── QMainWindow
│   ├── centralWidget: KioskWebView (QtWebKit) ← WebPageController
│   └── bottom QToolBar (44px, dark stylesheet)
│       [Home] [Remote] [Back] ──spacer── [WiFi icon] [DigitalClock] [Info]
│
├── C++ Controllers (plain QObjects, wired to widget signals)
│   ├── WebPageController  — loadLocal(), loadRemote(), reload(), goBack()
│   ├── NetworkController  — WiFi management via NetworkManager D-Bus
│   ├── SystemController   — reboot(), resetDefaults(), systemInfo()
│   └── WebsockServer      — debug WebSocket server (port 7070)
│
└── Widgets UI (src/)
    ├── digitalclock.h   — header-only QLabel clock, blinking colon, 1s timer
    ├── wifidialog.*     — WiFi config: scan, connect, forget, signal, restart
    ├── infodialog.*     — System info + reset defaults + reboot
    └── rebootdialog.*   — Reboot confirmation (opened from InfoDialog)
```

**Key C++ classes (all in `src/`):**
- `KioskWebView` (`kioskwebview.h`, header-only) — QWebView subclass that eats `MouseMove` events while a button is held, so WebCore never starts a drag, and eats all drag/drop events. Works around linuxfb pixel artifacts from `QSimpleDrag` (Debian's QtWebKit 5.212 ships `ENABLE_DRAG_SUPPORT=ON`). Text selection is sacrificed — acceptable for kiosk.
- `WebPageController` — owns KioskWebView + WebPage + CookieJar, exposes `webView()`. load/reload/back are Q_INVOKABLE; URL and loading state are Q_PROPERTY. Sets `AcceleratedCompositingEnabled = false` (required for linuxfb). Injects polyfills in two phases: JS (`js/polyfills.js`, `js/fetch.js`) on `javaScriptWindowObjectCleared` before page scripts run; CSS polyfills (css-vars-ponyfill, stickyfill, smoothscroll) activated on `loadFinished`.
- `WebPage` — simplified QWebPage subclass. Navigation network check, proxy, HTTP auth dialog, JS console/alert forwarding to debug server.
- `NetworkController` — WiFi management via NetworkManager D-Bus (system bus, raw QDBusInterface). Exposes signalLevel, connected, ssid, ipAddress, scanning, networks (QVariantList), error as Q_PROPERTY. Provides scan(), connectToNetwork(ssid, password), disconnectWifi(), forgetNetwork(ssid), restartWifi(). Uses 5-second polling + PropertiesChanged/AccessPointAdded/Removed D-Bus signals.
- `SystemController` — reboot, factory reset, system info string.
- `TestBrowserCookieJar` (`cookiejar.h/.cpp`) — disk-persisted cookies, 10-second debounced writes.
- `WebsockServer` — WebSocket server for remote debug, JS console and alert broadcast.
- `UnixSignalNotifier` — singleton converting SIGINT/SIGTERM to Qt signals for systemd shutdown.

## Qt Modules

core, gui, widgets, network, virtualkeyboard, websockets, dbus, webkit, webkitwidgets (QtWebKit 5.212 — system `libqt5webkit5-dev`, or a custom build via `WEBKIT_SOURCE_DIR`)

No quickwidgets/quickcontrols2 — the QML shell was removed.

## CLI Arguments

```
robot-browser <remote_url> [local_url]
```

Defaults to `http://127.0.0.1` for both if not provided.

## Resources (src/robot-browser.qrc)

Icons aliased under `:/images/` (favicon, home, store, back, info, wifi-off, wifi-0 through wifi-4). Polyfill scripts under `:/js/` (polyfills.js, fetch.js, css-vars-ponyfill.min.js, stickyfill.min.js, smoothscroll.min.js). No QML is bundled.

## Deployment

- **BBB:** Debian 12, linuxfb (no display server), systemd service. Input via evdev plugins with `grab=1` plus `QT_QPA_FB_NO_LIBINPUT=1` (see `rootfs/usr/lib/robot-browser/robotbrowser.sh`) — needed because the CX 2.4G receiver exposes a "Consumer Control" device with REL axes that Qt otherwise treats as a second mouse. See `rootfs/`, `docs/DEPLOYMENT.md`, `docs/BBB-SETUP.md`.
- **CM4:** Debian 12, Wayland via labwc + LightDM. Three session modes: desktop, Chrome kiosk, robot-browser kiosk. See `rootfs-cm4/` and `docs/DEPLOYMENT.md`.
- **WiFi:** Ezurio ST60-2230C (NXP 88W8997) over SDIO on both platforms. See `docs/WIFI-ROAMING.md`.

## Pending Work

- Virtual keyboard layout path may need updating for Debian 12 Qt packages (`QT_IM_MODULE=qtvirtualkeyboard` is set in the launch scripts)
- `src/qml/` is dead code from the pre-widgets UI and can be deleted

## Code Conventions

- C++14 standard, Qt 5.15 APIs only (no Qt 6)
- Qt signal/slot connections (prefer new-style `connect(&obj, &Class::signal, ...)`)
- Controllers expose state via Q_PROPERTY with NOTIFY signals; widgets connect to those signals directly
- UI is Qt Widgets only — do not reintroduce QML
- No address bar — kiosk mode with two fixed URLs only
