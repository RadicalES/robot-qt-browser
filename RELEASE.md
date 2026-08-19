# Release Notes

All notable changes to robot-browser are documented in this file.

---

## [3.1.1] - 2026-08-19

### Fixed

- **Scrollbars are back.** The Qt 6 port disabled them — reasonable for a page
  that fits the screen, wrong for the long forms these terminals show: there
  was nothing to say the page continued below the fold, and nothing to drag.
- **Sized for gloves.** Chromium's ~15px scrollbar is a mouse-sized target. The
  track is now 28px with an 18px thumb and a minimum thumb height, so a long
  page cannot shrink it to something a gloved hand cannot grab. Injected per
  profile rather than written into any page, since the transaction pages are
  not ours.


## [3.0.2] - 2026-08-18

### Added

- **`$` on the symbols keyboard page**, replacing the quote key. It was absent
  entirely, and there is no way to type a currency amount without it.
- **Show/Hide toggle on the WiFi password field.** A WPA key is long, typed on
  a virtual keyboard, in a packhouse — and a wrong one fails minutes later with
  "connection failed", which reads as a hardware fault rather than a typo.

### Changed

- Tighter toolbar spacing. With WiFi, LAN and the SCADA head all shown the row
  is nine buttons plus the clock, which overflowed a 720px panel. The padding
  gave way, not the 48px icons, so the touch targets are unchanged.
- Retired the BeagleBone `rootfs/` and the labwc `rootfs-cm4/` overlays, the
  leftover QML from the removed QML UI, and the QtWebKit-era documentation.
  All of it remains on the `webkit` branch.
- `README.md` and `CLAUDE.md` rewritten against the Qt 6 build; `WORKFLOW.md`
  rewritten for the dev → beta → main model, with CDN packages built from the
  release branch only.


## [3.0.1] - 2026-08-18

### Added

- **SCADA server status in the toolbar.** Mirrors the desktop tray indicator
  the kiosk session cannot show: grey when no server or service is detected,
  orange when the terminal is not provisioned, green when connected. Reads the
  same `/run/robot` files robot-scada-client publishes, so it needs no socket,
  no D-Bus, and not even the client package — a terminal without it shows grey.
  `ONLINE` with a stale `last_ok` reports as offline, so a dead link cannot
  read as working.
- **Provisioning MAC in the SCADA dialog.** Provisioning means reading the MAC
  off the screen; until now that needed a shell. Taken from the wired
  interface, which is what the server keys on.
- **The Robot mascot head**, traced from robot-scada-client's tray pixmaps into
  SVG so it stays sharp at both toolbar and dialog sizes. Heads the indicator,
  the SCADA dialog and the Info dialog.

### Changed

- Tighter toolbar spacing. With WiFi, LAN and the SCADA head all shown the row
  is nine buttons plus the clock, which overflowed a 720px panel. The padding
  gave way, not the 48px icons — the touch targets are unchanged.


## [3.0.0] - 2026-08-18

Rendering engine replaced. Nothing about the 2.x line carries over.

### Engine

- Replaced QtWebKit 5.212 with **Qt 6 + QtWebEngine** (Chromium 102)
- Removed all JS/CSS polyfills: `fetch`, `Object.values`/`entries`,
  `URLSearchParams`, CSS custom properties, `position: sticky` and smooth
  scroll are native in Chromium, so the shims and the two-phase injection that
  delivered them are gone
- Replaced the hand-rolled cookie jar with a persistent `QWebEngineProfile`
- Build moved from qmake to CMake — Debian's Qt 6 ships no aarch64 mkspec

### Platforms

- **Dropped the BeagleBone Black (armhf) target.** QtWebEngine needs EGL/GLES
  and cannot run on linuxfb
- Runs on CM4 and Raspberry Pi 5 (arm64, Bookworm) under an X11 kiosk session.
  Wayland is not usable: Qt 6 refuses the client-side virtual keyboard there,
  and these terminals are touch-only

### On-screen keyboard

- Restored the T420 keyboard — three pages (letters, numbers, symbols), the
  custom red-on-dark style, one character per key, touch-sized targets
- Hosted in-app, because Debian's Qt 6 QtVirtualKeyboard has no desktop
  integration compiled in and creates no panel of its own
- Added a toolbar button to show and hide it manually
- Fixed four separate defects that each produced "the keyboard does not come
  up": the input method following the active window across dialogs, dialogs
  opening over a live keyboard, dialogs retaining focus after hiding, and the
  software input panel needing a second tap

### Networking

- Added wired network support with its own toolbar icon, distinguishing "no
  cable" from "cable connected, no address"
- Added IPv4 settings — DHCP or fixed address, netmask, gateway, DNS — shared
  between wired and wireless
- Rate-limited WiFi scanning against NetworkManager's `LastScan`
- The provisioning hotspot is no longer reported as a network connection
- Ships a polkit rule so the kiosk user can apply network changes

### Interface

- Toolbar and dialogs sized for touch; icons redrawn flat in a single tone
- Load progress shown on the page's bottom edge
- Offline reported inline instead of in a modal dialog

### Packaging

- Runs standalone from `/etc/robot-browser/browser.config`: URLs, and
  `NETWORK_WIFI`/`NETWORK_LAN` to select what a terminal offers (`auto`, `on`,
  `off`). Command-line arguments still override, so a provisioning layer
  integrates without this package knowing it exists
- The systemd service is no longer enabled on install — session-launched
  terminals would get a second instance fighting for the display
- Published to https://packages.radicales.net for bookworm/arm64

## [2.1.0] - 2026-03-02

### Added
- ce1c622 Rename RBrowser to robot-browser, add docs and workflow
- f4b7f0f Add JS and CSS polyfills for QtWebKit 5.212 compatibility
- 9ac6e89 Add Debian packaging and build-deb script
- 57380ae Standardize rootfs layout to match Debian package
- ef55ab9 Add overlay event filter and graceful virtual keyboard loading
- 34050a3 Add QML WiFi configuration UI via NetworkManager D-Bus

### Changed
- a93d9f3 Rewrite UI shell in QML, strip unused browser code
- 9a8953f Fix deprecated Qt 5.15 APIs and add Docker cross-compilation for BBB
- 2f3dcc0 Add Docker cross-compilation for Raspberry Pi CM4 (arm64)
- e690883 Move source code, QML, and images into src/ directory
- db4fe98 Remove legacy wpa_supplicant C library
- 32a6b77 Update rootfs for Debian 12 (BBB deployment)
- eb62218 Add CM4 deployment files (Wayland/labwc kiosk session)

## [1.0.0] - 2025-01-01

### Added
- Initial release targeting Qt 5.9 with Widgets-based UI
- Native wpa_supplicant C library for WiFi
- Manual deployment via rootfs overlay
