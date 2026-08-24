# Release Notes

All notable changes to robot-browser are documented in this file.

---

## [3.6.2] - 2026-08-24

### Fixed

- **A terminal switched to Secure while running had no way to sign off.** The
  lock button was created only when security was on *at startup*, so a terminal
  that booted Open and was later marked Secure locked itself correctly and then
  offered nothing to lock it again. Security is a setting that changes while the
  terminal runs, so nothing may depend on its value at startup.
- **Home, Remote and Back worked while the terminal was locked**, on the page
  behind the lock — so anybody could navigate a terminal they had not signed on
  to, and the operator did not come back to what they left. They are disabled
  while locked. WiFi, LAN, SCADA state and Info stay reachable on purpose:
  those fix a terminal rather than drive it.


## [3.6.1] - 2026-08-24

### Changed

- **The package describes what it actually is.** The description had been
  written for a different product — "Kiosk browser for embedded Linux robots …
  for Raspberry Pi CM4 and Raspberry Pi 5" — naming neither amd64 nor the
  ITPC-200, and none of what a buyer chooses it for. This is not only apt
  metadata: the catalogue on `cdn.radsys.io` builds its download listing from
  this field, so editing the website would not have changed it.
- **The package says which terminals it runs on**, in `X-Radical-Products`, so
  the catalogue can show it without a second list on the website that drifts.


## [3.6.0] - 2026-08-24

### Added

- **A terminal can lock, and ask who is signing on.** A terminal in a packhouse
  is a shared machine: it runs unattended between transactions, and whoever
  walks past gets whatever the last operator left open. It now behaves like a
  desktop — it asks who you are before showing the page, and locks itself again
  when nobody has touched it.
- **The site decides, on the server.** A terminal marked **Secure Terminal**
  has `security=SECURE` in the setup `robot-scada-client` publishes, and that
  is what turns this on. Switching it back to Open removes the lock within
  seconds, with no restart and no visit — the setting is followed while the
  terminal runs, not read once at boot. `WB_SECURITY` only decides for a
  terminal the server has never spoken to.
- **A worker signs on with one value**, keyed on the pad or presented as a
  card. Cards and scanned badges arrive from `wsrobot`'s websocket, which
  serves every reader on the terminal over one socket, and the `[0]` a
  multi-reader terminal prefixes is stripped — it is decoration on a card
  number, not a different card. The card option only appears when a reader is
  actually connected.
- **Signing on is a check, not an authentication.** It posts the Robot API's
  `publishLogon` and reads the answer; a refusal shows the server's own wording,
  because "Invalid Card Number / Worker not found" is what whoever the operator
  calls will recognise. Being refused and being unable to ask are different
  states and read differently.
- **Locking signs the worker off**, with `publishLogoff` — on the idle timeout,
  on the toolbar's lock button, and on shutdown. A terminal switched off
  mid-shift would otherwise leave a session nobody closes, and the record would
  say the worker never went home.
- **The lock timeout is in Settings**, defaulting to five minutes. Whether a
  terminal locks is not a setting there: a terminal that could switch its own
  lock off would not be a secure terminal.

### Changed

- The lock covers the page and not the toolbar. WiFi, LAN, SCADA state and Info
  stay reachable without signing on, because a terminal whose network has died
  has to be fixable at the terminal, and none of those show a customer's data.
- The page keeps its state behind the lock, so unlocking returns the operator to
  exactly where they were. An idle lock never costs somebody their work.

### Fixed

- **A page could put a dialog on top of the lock.** A page asking for HTTP
  credentials does it in a modal dialog, so a terminal meant to be asking who
  you are instead showed a password box for a page nobody had signed on to see.
  The request is refused while locked and the page reloaded on unlock, when
  there is somebody to answer it.
- **The keyboard swallowed every keystroke on the lock screen.** The Keyboard
  button focused the web view unconditionally, and while locked that view is
  hidden — so the panel appeared and the typing went into a page nobody could
  see, which reads exactly like a keyboard that does not work.
- **The idle countdown ran from the last touch rather than from the sign-on**,
  so a terminal observed on hardware locked three minutes after a worker signed
  on instead of five. The five minutes belong to their session.


## [3.5.0] - 2026-08-23

### Added

- **A device profile is pickable from Settings, and it sticks.** The developer
  build exists so a page can be seen at a terminal's own viewport; switching
  terminal meant restarting from a shell with a different `--profile`. The
  Settings dialog lists every device by name and size, and choosing one
  restarts the browser into it.
- **`--profile=user`**, which means "whatever Settings last saved, `desktop`
  until it has saved anything". The menu entry passes it, so a device chosen in
  Settings survives a restart; naming any real profile still outranks it,
  because a terminal is what its launcher says it is.
- **The device is named in the title bar**, with the scale when it is drawn
  smaller than 1:1 — `Robot Browser — Robot-T440 (720 x 1280 portrait at 78%)`.
  Comparing a page across terminals means several of these open at once, and
  two portrait windows of similar size are not told apart by looking at them.

### Changed

- **A device profile reproduces the viewport and nothing else.** No WiFi
  indicator for a radio the PC does not have, no SCADA head reading a
  `/run/robot` that is not there, no on-screen keyboard over a real keyboard.
  The keyboard is still available with `--keyboard=full` to see what it costs.
- **The menu entry is `io.radsys.RobotBrowser`**, matching the domain the
  product is published under, and the icon is installed and named for the same
  ID.
- **Setup instructions point at `cdn.radsys.io`.** The same bucket under a new
  name; `packages.radicales.net` still serves it, so terminals already in the
  field keep updating.

### Fixed

- **The saved profile was applied after the fit-to-screen calculation**, so a
  saved T440 opened at a full 720x1280 on a 1080-tall screen while
  `--profile=t440` was drawn at 78% — the same profile behaving two different
  ways depending on how it was chosen.
- **The application set no window icon at all**, so a taskbar had nothing to
  read from the window. It sets one now, and sets the desktop file name too:
  Wayland ignores the window icon and matches the surface's app-id to a
  `.desktop` file instead.
- **The launcher icon was unreadable to GTK.** `robot-head.svg` opened with a
  700-byte comment, which pushed `<svg` past the bytes gdk-pixbuf sniffs to
  identify a file, so every GTK consumer rejected it and the menu drew nothing.
  Qt has its own parser and never noticed.
- **`applicationVersion` was hardcoded to "2.1"**; it comes from `APP_VERSION`.


## [3.4.0] - 2026-08-22

### Added

- **Run profiles.** `--profile=kiosk|t430|t431|t432|t440|itpc200|desktop` says what
  this instance is meant to be, instead of a growing pile of flags a caller has
  to get consistently right. A device profile reproduces that terminal on a PC —
  its panel's exact viewport, the network controls it actually has, and none of
  the buttons that would act on the developer's own machine. Written up for
  people outside the company in `docs/WEBAPP-DEVELOPERS.md`.

- **Every profile geometry measured on hardware.** Not one matched what its
  panel overlay name suggested: the T430 is 800x480, a T431 1024x600, a T432
  1280x800 (a portrait panel the session rotates), a T440 720x1280 and an
  ITPC-200 1920x1080.

- **Scaled mode.** A T440's 800-wide portrait panel does not fit above a taskbar
  on a 1080-tall monitor. Rather than crop the viewport — quietly changing the
  thing a developer is trying to look at — the whole device is drawn smaller and
  the page still lays out at the device's size. `--scale=1|fit|N` overrides it.

- **Three keyboards, chosen by `--keyboard=auto|full|standard|narrow|off`.**
  `auto` asks the panel: a short landscape screen gets a four-across keyboard
  docked down the side, because ten keys across a third of 1024px is 34px each
  and no scaling fixes that; a wide screen gets a single keypad with letters,
  numpad and symbols side by side and nothing behind a mode key; everything else
  keeps the ten-across layout. Sized from the panel's physical dimensions where
  it reports them — a finger is the same size whatever the display is.

- **A Settings dialog**, reached from System Info on a PC, editing both URLs and
  saving them per user in `~/.config/robot-browser/browser.config`. Never on a
  terminal: it is told what it is by the file its deployment controls, and a
  kiosk whose address an operator can retype is not a kiosk.

- **Desktop builds.** bookworm, trixie, Ubuntu 24.04 and Ubuntu 26.04, each
  built inside its own distro because each ships a different Qt 6 and a binary
  built against one does not run on another. The package version carries the
  suite it was built for. `docker/run-desktop.sh` installs the built package in
  a bare image of that distro and puts a window on the local desktop, so the
  dependency list is under test too.

- **A menu entry** on a PC install, in `/usr/share/applications` for every user
  on the machine, with the device profiles as right-click actions.

- **The terminal's MAC in the LAN dialog**, connected or not. It is what a site
  needs to write a DHCP reservation, which is exactly the job someone is doing
  when the address is wrong or the cable is not in yet.

- **Contact details in System Info.** A terminal in a packhouse is a long way
  from whoever supports it, and that dialog is where somebody already goes when
  something is wrong.

### Changed

- **The toolbar follows the panel** — a tenth of its height, capped at 60px on a
  landscape screen where height is shared with the page and the keyboard, and
  unchanged at 76px on a portrait panel or a desktop.

- **Dialogs size and centre on the window they belong to**, not on the screen.
  On a terminal those are the same thing; in a window they are not, and the old
  arithmetic gave an 1800px-wide dialog over an 800px window.

- **The `[Remote]` icon** is Material Symbols' "dataset". It was a cloud on a
  122x79 viewBox, so it rendered at 48x30 beside solid 48x48 neighbours and read
  as a missing icon rather than a faint one.

- **The orange robot head** is the product's mark: the application icon and the
  face in System Info. That dialog used to recolour it from the SCADA
  connection, so opening System Info on an unprovisioned terminal showed a
  warning about something nobody had asked about.

### Fixed

- **The keyboard would not stay hidden.** Dismissing it while a text field still
  held focus made it flicker and come straight back — the field is an
  input-method client and Qt re-shows the panel for it. The focused element is
  blurred first.

- **`auto` now means what it says for WiFi.** It is documented as "offer it when
  NetworkManager reports the device", and the LAN button always worked that way,
  but the WiFi button was shown unconditionally — so an ITPC-200, which has no
  radio at all, offered a WiFi dialog with nothing in it.

- **`--profile=t440` on a machine with a config file** no longer lands on
  `127.0.0.1`. The package installs `browser.config` everywhere, its URL lines
  are commented out now, and unset means "nobody has said" rather than a value
  that outranks the profile.

### Notes

Window decorations are the compositor's business. `Qt::FramelessWindowHint` and
`Qt::CustomizeWindowHint` change nothing under labwc and cost a stuck WiFi
dialog and a modal child dialog that opened behind its parent. The titlebar
button layout belongs in the rootfs `rc.xml`.


## [3.2.2] - 2026-08-21

### Added

- **`--windowed[=WxH]` and `--no-toolbar`.** The browser is a kiosk by default —
  fullscreen, toolbar, the whole panel — but it is also the best way to show the
  terminal's own WiFi setup page from a desktop session, where a fullscreen
  window with no way out traps whoever opened it and the kiosk toolbar means
  nothing. Both flags are off by default, so kiosk behaviour is unchanged.


## [3.2.1] - 2026-08-20

### Changed

- **Password reveal moved inside the WiFi password field.** A separate
  Show/Hide button beside the field is not the convention; the eye belongs at
  the trailing edge. Uses `QLineEdit::addAction` with `TrailingPosition`, so the
  icon sits inside the frame and the row loses a widget. The field re-masks
  whenever the password row is reopened, so a key left revealed cannot carry
  over to the next network.


## [3.2.0] - 2026-08-19

### Fixed

- **The virtual keyboard failed to load on a clean install.** The package did
  not depend on `qml6-module-qtquick-layouts`, `-window`, `-controls` or
  `-templates`, so on a freshly imaged terminal the keyboard's QML aborted with
  `module "QtQuick.Layouts" is not installed`. The panel then held its space
  without drawing anything: half the screen grey, the load bar stranded in the
  middle, and no keyboard. Development machines had the modules from other work,
  which is why it only appeared on a bare device.


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
