# RBrowser Deployment Guide

This document describes how RBrowser is deployed and runs on its two target platforms.

## Target Platforms

| | BeagleBone Black (BBB) | Raspberry Pi CM4 |
|---|---|---|
| **SoC** | AM335x ARM Cortex-A8 | BCM2711 ARM Cortex-A72 |
| **Architecture** | armhf (32-bit) | arm64 (64-bit) |
| **RAM** | 512 MB | 1–8 GB |
| **GPU** | None (SGX530 unusable) | VideoCore VI |
| **OS** | Debian 12 (Bookworm) minimal | Debian 12 (Bookworm) / Raspberry Pi OS |
| **Display server** | None (linuxfb) | labwc (Wayland compositor) via LightDM |
| **Qt platform plugin** | `linuxfb` | `wayland` |

## Cross-Compilation

Both platforms are cross-compiled from an x86_64 host using Docker containers with Debian 12 and the target architecture's Qt 5.15 development packages.

```sh
# BeagleBone Black (armhf)
./docker/build-bbb.sh

# Raspberry Pi CM4 (arm64)
./docker/build-cm4.sh
```

Build output:
- `build-bbb/RBrowser` — ELF 32-bit ARM, ~172K
- `build-cm4/RBrowser` — ELF 64-bit ARM aarch64, ~231K

Both directories are gitignored (`*build-*` pattern in `.gitignore`).

## Platform 1: BeagleBone Black

### How it runs

The BBB runs a minimal Debian 12 installation with **no display server** (no X11, no Wayland). RBrowser draws directly to the Linux framebuffer using Qt's `linuxfb` platform plugin. A systemd service starts the application automatically at boot.

```
systemd multi-user.target
  └── browser.service
        └── robotbrowser.sh
              └── RBrowser (QT_QPA_PLATFORM=linuxfb)
                    ├── QWebView → renders web content
                    └── QML overlay → bottom bar, popups, virtual keyboard
```

### Boot sequence

1. System boots to `multi-user.target` (no graphical target)
2. `browser.service` starts after `network-online.target`
3. `robotbrowser.sh` reads `/etc/formfactor/appconfig` for URL and display orientation
4. Environment variables configure Qt for framebuffer rendering and evdev input
5. RBrowser launches fullscreen on the framebuffer

### Key environment variables

| Variable | Value | Purpose |
|---|---|---|
| `QT_QPA_PLATFORM` | `linuxfb:rotation=270` | Framebuffer rendering with rotation |
| `QT_QPA_FB_DRM` | `1` | Use DRM/KMS for framebuffer access |
| `QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS` | `/dev/input/touchscreen0:rotate=270` | Touchscreen input with rotation |
| `QT_QPA_GENERIC_PLUGINS` | `evdevtouch:/dev/input/touchscreen0` | Evdev touch plugin |
| `QT_IM_MODULE` | `qtvirtualkeyboard` | On-screen keyboard |
| `QT_VIRTUALKEYBOARD_LAYOUT_PATH` | `/home/root/RobotBrowser/layouts` | Custom keyboard layouts |

### Input devices

Udev rules in `/etc/udev/rules.d/99-robot-input.rules` create stable symlinks:
- `/dev/input/touchscreen0` — touchscreen device
- `/dev/input/keyboard0` — physical keyboard (if present)

The `ATTRS{name}` patterns in the udev rules must be adjusted to match the actual hardware. Inspect connected devices with:

```sh
cat /proc/bus/input/devices
```

### File locations on target

```
/home/root/RobotBrowser/
├── RBrowser                 # Application binary
├── robotbrowser.sh          # Startup script
└── layouts/                 # Virtual keyboard layouts

/etc/formfactor/appconfig    # URL and display orientation config
/etc/systemd/system/browser.service
/etc/udev/rules.d/99-robot-input.rules
```

### Installation

Copy the `rootfs/` tree onto the BBB and run the install script as root:

```sh
# Copy build artifact into rootfs
cp build-bbb/RBrowser rootfs/home/root/RobotBrowser/

# On the BBB (via SSH or serial):
sudo ./rootfs/install.sh
sudo reboot
```

The install script:
1. Installs Qt 5.15 runtime packages and NetworkManager
2. Copies the binary, startup script, and layouts
3. Installs the systemd service and udev rules
4. Sets `multi-user.target` as default (disables any desktop environment)

### Configuration

Edit `/etc/formfactor/appconfig` on the target:

```sh
# URL to load on startup
WB_LOAD_URL=http://192.168.100.1/transaction

# Display orientation: portrait or landscape
WB_LAYOUT=portrait
```

## Platform 2: Raspberry Pi CM4

### How it runs

The CM4 uses the existing LightDM + labwc (Wayland compositor) infrastructure from the robot-t430 platform. RBrowser runs as a **third session option** alongside the desktop and Chrome kiosk modes. Qt renders via its `wayland` platform plugin under the labwc compositor.

```
LightDM (autologin → robot user)
  └── LXDE-pi-labwc-rbrowser session
        └── labwc -C /etc/xdg/labwc-rbrowser
              ├── kanshi (display config)
              └── rbrowser-kiosk.sh
                    └── RBrowser (QT_QPA_PLATFORM=wayland)
                          ├── QWebView → renders web content
                          └── QML overlay → bottom bar, popups, virtual keyboard
```

### Session selection

The active session is controlled by which config file is symlinked to `/etc/lightdm/lightdm.conf`:

| Mode | Config file | Command to activate |
|---|---|---|
| Desktop | `lightdm.desktop.conf` | `sudo ln -sf /etc/lightdm/lightdm.desktop.conf /etc/lightdm/lightdm.conf` |
| Chrome kiosk | `lightdm.robot.conf` | `sudo ln -sf /etc/lightdm/lightdm.robot.conf /etc/lightdm/lightdm.conf` |
| **RBrowser kiosk** | `lightdm.rbrowser.conf` | `sudo ln -sf /etc/lightdm/lightdm.rbrowser.conf /etc/lightdm/lightdm.conf` |

After changing the symlink, reboot to apply.

### Boot sequence

1. System boots to `graphical.target`
2. LightDM starts and auto-logs in the `robot` user
3. The `LXDE-pi-labwc-rbrowser` wayland session starts
4. `labwc-rbrowser` script launches the labwc compositor with the `/etc/xdg/labwc-rbrowser` config
5. labwc autostart runs `kanshi` (display configuration) and `rbrowser-kiosk.sh`
6. `rbrowser-kiosk.sh` reads `/etc/formfactor/appconfig` and launches RBrowser with `QT_QPA_PLATFORM=wayland`

### labwc kiosk configuration

The `labwc-rbrowser` config directory provides a locked-down Wayland session:

- **rc.xml** — No window titlebars, no task switching shortcuts, RBrowser forced fullscreen. Only `Ctrl+Alt+Delete` (shutdown) and power key are bound. Touchscreen device mappings for DSI displays are included.
- **environment** — Sets `QT_WAYLAND_DISABLE_WINDOWDECORATION=1` and virtual keyboard module.
- **autostart** — Launches kanshi and the RBrowser kiosk script.

### File locations on target

```
/etc/lightdm/lightdm.rbrowser.conf

/usr/share/wayland-sessions/LXDE-pi-labwc-rbrowser.desktop
/usr/bin/labwc-rbrowser

/etc/xdg/labwc-rbrowser/
├── autostart
├── environment
└── rc.xml

/usr/local/bin/rbrowser-kiosk.sh

/home/root/RobotBrowser/
├── RBrowser                 # Application binary
└── layouts/                 # Virtual keyboard layouts

/etc/formfactor/appconfig    # URL and display orientation config
```

### Installation

Copy the `rootfs-cm4/` files onto the CM4:

```sh
# Copy the binary
cp build-cm4/RBrowser /home/root/RobotBrowser/
chmod +x /home/root/RobotBrowser/RBrowser

# Copy rootfs-cm4 overlay
sudo cp -r rootfs-cm4/* /

# Make scripts executable
sudo chmod +x /usr/bin/labwc-rbrowser /usr/local/bin/rbrowser-kiosk.sh

# Install Qt runtime packages
sudo apt-get install -y --no-install-recommends \
    libqt5core5a libqt5gui5 libqt5widgets5 libqt5network5 \
    libqt5qml5 libqt5quick5 libqt5quickwidgets5 \
    libqt5quickcontrols2-5 libqt5quicktemplates2-5 \
    libqt5webkit5 libqt5websockets5 libqt5virtualkeyboard5 \
    qml-module-qtquick2 qml-module-qtquick-controls2 \
    qml-module-qtquick-layouts qml-module-qtquick-window2 \
    qtvirtualkeyboard-plugin

# Activate RBrowser session
sudo ln -sf /etc/lightdm/lightdm.rbrowser.conf /etc/lightdm/lightdm.conf
sudo reboot
```

### Configuration

Edit `/etc/formfactor/appconfig` on the target (same as BBB):

```sh
WB_LOAD_URL=http://192.168.100.1/transaction
WB_LAYOUT=portrait
```

## Platform Comparison

| Aspect | BBB (linuxfb) | CM4 (Wayland) |
|---|---|---|
| Display server | None | labwc (Wayland) |
| Qt platform plugin | `linuxfb` | `wayland` |
| GPU acceleration | No | Yes (VideoCore VI) |
| Touchscreen handling | evdev via Qt | libinput via labwc |
| Display rotation | Qt `linuxfb:rotation=` | Compositor / kanshi |
| Session management | systemd service | LightDM + labwc session |
| Switching modes | N/A (single purpose) | Symlink `/etc/lightdm/lightdm.conf` |
| Service restart | `sudo systemctl restart browser` | Switch to desktop, switch back |

## Application Configuration

Both platforms read `/etc/formfactor/appconfig`:

| Setting | Description | Example |
|---|---|---|
| `WB_LOAD_URL` | Primary URL loaded at startup | `http://192.168.100.1/transaction` |
| `WB_LAYOUT` | Display orientation | `portrait` or `landscape` |

The application accepts the remote URL as its first argument and an optional local URL as the second argument. If no local URL is provided, it defaults to `http://127.0.0.1`.

## Networking

Both platforms use **NetworkManager** for network management. WiFi control from within the application will use the NetworkManager D-Bus API (implementation pending).

## Troubleshooting

### BBB: Black screen after boot

Check if the service is running and inspect logs:

```sh
systemctl status browser
journalctl -u browser -f
```

Verify the framebuffer is available:

```sh
ls -l /dev/fb0
cat /sys/class/graphics/fb0/modes
```

### CM4: RBrowser session not appearing

Verify the session file is installed:

```sh
ls -l /usr/share/wayland-sessions/LXDE-pi-labwc-rbrowser.desktop
```

Check LightDM logs:

```sh
cat /var/log/lightdm/lightdm.log
```

### Both: Touchscreen not working

List input devices and verify udev symlinks:

```sh
cat /proc/bus/input/devices
ls -l /dev/input/touchscreen0
```

On BBB, update the `ATTRS{name}` pattern in `/etc/udev/rules.d/99-robot-input.rules` to match your hardware, then reload:

```sh
sudo udevadm control --reload-rules
sudo udevadm trigger
```

On CM4, touchscreen mapping is handled in `/etc/xdg/labwc-rbrowser/rc.xml` via `<touch>` entries.
