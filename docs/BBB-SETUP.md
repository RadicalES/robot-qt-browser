# BeagleBone Black Setup Notes

Target: BeagleBoard.org Debian Bookworm Base Image 2026-02-12, 1.8GB eMMC.

## Base Image

- Debian 12 (Bookworm) armhf
- Kernel 6.x (TI)
- eMMC partitions: 36MB FAT32 boot, 32MB swap, 1.7GB ext4 root

## Packages Removed (not needed for kiosk)

```bash
apt remove --purge -y gcc g++ gcc-12 g++-12 libgcc-12-dev cpp cpp-12 \
    nginx nginx-common nginx-core \
    libpython3-dev python3-dev
apt autoremove --purge -y
apt clean
```

## Disk Cleanup

```bash
# Remove BBB dev sources
rm -rf /opt/source

# Clean apt cache
apt clean

# Vacuum journal logs
journalctl --vacuum-size=8M
```

## Disable Persistent Journal Logging

Saves ~60MB and reduces eMMC wear. Logs kept in RAM only (max 4MB).

```bash
mkdir -p /etc/systemd/journald.conf.d
cat > /etc/systemd/journald.conf.d/no-persist.conf << 'EOF'
[Journal]
Storage=volatile
RuntimeMaxUse=4M
EOF

rm -rf /var/log/journal
systemctl restart systemd-journald
```

## Swap

- 32MB partition (`/dev/mmcblk1p2`)
- zram0 (~240MB compressed in-memory swap, enabled by default via zram-tools)

## Input Devices

The CX 2.4G wireless receiver exposes multiple input interfaces:
- `event1` — keyboard (HID)
- `event2` — mouse (relative axes)
- `event4` — consumer control (has REL axes, can cause duplicate mouse events)
- `event5` — system control

**Important:** Use `grab=1` on the evdevmouse plugin to prevent duplicate events from the consumer control interface:

```bash
export QT_QPA_GENERIC_PLUGINS="evdevmouse:/dev/input/by-id/usb-CX_2.4G_Receiver-if01-event-mouse:grab=1,evdevkeyboard"
export QT_QPA_FB_NO_LIBINPUT=1
```

## Qt/WebKit Drag Fix (linuxfb)

Debian's QtWebKit 5.212 has `ENABLE_DRAG_SUPPORT=ON`, which causes broken drag images (pixel artifacts, no-go cursor) when clicking links on linuxfb. The `KioskWebView` subclass suppresses this by eating mouse-move events while a button is held, preventing WebCore from detecting drag gestures.

## Disk Usage After Cleanup

```
Filesystem      Size  Used Avail Use%
/dev/mmcblk1p3  1.7G  1.3G  344M  79%
```

## TODO

- [ ] Create automated setup script or custom SD card image
- [ ] Consider removing more unused packages (perl, python3 if not needed)
- [ ] Evaluate `/usr/include` removal (26MB of dev headers)
