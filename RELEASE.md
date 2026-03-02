# Release Notes

All notable changes to robot-browser are documented in this file.

---

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
