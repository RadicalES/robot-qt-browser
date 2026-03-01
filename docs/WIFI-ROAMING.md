# WiFi Roaming Configuration Guide

This document covers WiFi roaming configuration for the Robot Kiosk Browser
deployed in fruit packhouse environments on BeagleBone Black (BBB) and
Raspberry Pi CM4 hardware.

## Problem Statement

The kiosk terminals are mounted on mobile carts or movable stations in large
packhouses. As they move between areas, they must seamlessly roam between
Ubiquiti UniFi access points without dropping the transaction connection.
Default Linux WiFi behaviour is extremely conservative — it holds onto a weak
AP until the connection is essentially lost, causing multi-second dropouts that
break the transaction workflow.

## Hardware

### WiFi Module

Both platforms use the **Ezurio ST60-2230C-S** (SDIO variant) with antenna
diversity:

| Spec | Value |
|------|-------|
| Manufacturer | Ezurio (formerly Laird Connectivity) |
| Chipset | NXP 88W8997 |
| Interface | SDIO |
| WiFi standard | 802.11a/b/g/n/ac (WiFi 5) |
| MIMO | 2x2 MU-MIMO |
| Bands | Dual-band 2.4 GHz + 5 GHz |
| Max rate | 866.7 Mbps (5 GHz, 80 MHz, MCS9) |
| Antenna | Diversity (2 antenna connectors) |
| Temperature | -40 to +85 C (industrial) |
| Roaming standards | 802.11r, 802.11k, 802.11v (all supported in chipset/firmware) |

The same module and driver stack is used on both BBB and CM4, keeping the
WiFi configuration identical across platforms.

### Why Not the CM4 Onboard WiFi?

The Raspberry Pi CM4 includes an onboard Broadcom BCM43455 WiFi chip using
the `brcmfmac` kernel driver. This chip is **not suitable** for packhouse
roaming for the following reasons:

1. **No 802.11r support.** The BCM43455 firmware does not implement Fast BSS
   Transition. Worse, there is a documented firmware bug
   (https://bugs.launchpad.net/raspbian/+bug/1929746) where the chip **fails
   to associate** with APs that have 802.11r enabled. In a packhouse with
   Ubiquiti APs and 802.11r turned on, the onboard WiFi may not connect at
   all.

2. **Driver suppresses roaming signals.** The `brcmfmac` driver performs
   internal roaming decisions and suppresses `signal_change` notifications
   to wpa_supplicant. This means wpa_supplicant never learns that the signal
   has degraded and never triggers a background scan or roam. Setting the
   module parameter `roamoff=1` disables internal roaming but does not add
   802.11r/k/v support — it just makes the client sit on a dying AP until
   disconnect.

3. **Single antenna, no diversity.** The onboard chip has a single PCB
   antenna with no external connector. In a packhouse environment with metal
   shelving, cold rooms, and conveyor systems, antenna diversity is essential
   for reliable connectivity.

4. **Limited 802.11k/v support.** The `brcmfmac` driver has incomplete
   support for 802.11k neighbor reports and 802.11v BSS transition
   management. Without these, the client cannot benefit from AP-assisted
   roaming even when the Ubiquiti APs provide these services.

**The CM4 onboard WiFi should be disabled** when using the external
ST60-2230C module to avoid interference and driver conflicts:

```sh
# Disable onboard WiFi via device tree overlay
# Add to /boot/config.txt:
dtoverlay=disable-wifi
```

Alternatively, blacklist the driver:

```sh
echo "blacklist brcmfmac" | sudo tee /etc/modprobe.d/blacklist-brcmfmac.conf
```

### Access Points

Ubiquiti UniFi APs with the following standards support:

| Standard | Purpose | UniFi support |
|----------|---------|---------------|
| 802.11k | Neighbor reports — client learns which APs are nearby | Yes |
| 802.11v | BSS transition management — AP suggests client should roam | Yes |
| 802.11r | Fast BSS Transition — pre-authentication reduces handoff to ~50ms | Yes |

## Linux Driver Selection

Two driver options exist for the NXP 88W8997 on Linux:

### Option A: Mainline mwifiex_sdio (kernel built-in)

- Module: `mwifiex_sdio`
- Available in Linux kernel since ~4.5
- Debian 12 kernel (6.1) includes it
- 802.11r: Supported via wpa_supplicant
- 802.11k/v: Partial support
- Known issues: Reconnection loops, firmware crash bugs, WiFi drops under
  heavy traffic

### Option B: NXP out-of-tree mlan driver (recommended)

- Source: https://github.com/nxp-imx/mwifiex
- Full 802.11r/k/v support (documented in NXP Application Note AN14212)
- Actively maintained, fixes land faster than mainline
- Requires blacklisting mainline modules:
  ```
  blacklist mwifiex_sdio
  blacklist mwifiex
  ```
- Must be built from source against the running kernel headers

**Recommendation:** Use the NXP out-of-tree mlan driver for production.
The mainline mwifiex_sdio is acceptable for development and initial testing
but has known stability issues that matter in a packhouse environment.

### Driver installation (NXP out-of-tree)

```sh
# On the target device
apt-get install -y linux-headers-$(uname -r) build-essential git

git clone https://github.com/nxp-imx/mwifiex.git
cd mwifiex/mxm_wifiex/wlan_src
make build KERNELDIR=/lib/modules/$(uname -r)/build
sudo make install KERNELDIR=/lib/modules/$(uname -r)/build

# Blacklist mainline driver
echo -e "blacklist mwifiex_sdio\nblacklist mwifiex" | \
    sudo tee /etc/modprobe.d/blacklist-mwifiex.conf

# Load NXP driver
sudo modprobe moal
```

### Firmware

Ensure the latest NXP firmware is installed:

```
/lib/firmware/nxp/sdio8997_combo_v4.bin
```

Firmware files are available from the NXP GitHub releases or the
Ezurio Sterling-60 release packages:
https://github.com/Ezurio/Sterling-60-Release-Packages/releases

### Power management

WiFi power saving causes intermittent disconnections in industrial
environments. Disable it at boot:

```sh
# /etc/NetworkManager/dispatcher.d/pre-up.d/disable-wifi-powersave
#!/bin/sh
iw dev wlan0 set power_save off
```

Or via a systemd unit:

```ini
# /etc/systemd/system/wifi-powersave-off.service
[Unit]
Description=Disable WiFi power saving
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/sbin/iw dev wlan0 set power_save off
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

## wpa_supplicant Roaming Configuration

### Overview

The roaming strategy has three layers:

1. **Client-side bgscan** — wpa_supplicant periodically scans for better APs
   in the background, increasing scan frequency when signal is weak
2. **802.11r Fast Transition** — pre-authentication with the target AP reduces
   handoff time from ~400ms to ~50ms
3. **AP-side Min-RSSI** — Ubiquiti APs kick clients whose signal drops below
   a threshold, forcing sticky clients to roam

### wpa_supplicant configuration

Create `/etc/wpa_supplicant/wpa_supplicant-wlan0.conf`:

```ini
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=0
update_config=1
country=ZA

# Background scanning:
#   simple:<short_interval>:<rssi_threshold>:<long_interval>
#   - Scan every 10s when signal drops below -65 dBm
#   - Scan every 120s when signal is good
bgscan="simple:10:-65:120"

network={
    ssid="PackhouseNet"
    psk="your-password-here"

    # Fast Transition with WPA-PSK fallback
    # FT-PSK: uses 802.11r when AP supports it
    # WPA-PSK: falls back to standard handshake if FT fails
    key_mgmt=FT-PSK WPA-PSK

    # Management Frame Protection (must match AP setting)
    # 0=disabled, 1=optional (UniFi default), 2=required
    ieee80211w=1

    # Opportunistic Key Caching — reduces roaming latency
    # even without 802.11r by caching PMKs across APs
    proactive_key_caching=1
}
```

### Build requirements

wpa_supplicant must be compiled with these options (Debian 12 default
wpa_supplicant includes them):

```
CONFIG_IEEE80211R=y        # 802.11r Fast Transition
CONFIG_BGSCAN_SIMPLE=y     # Background scanning
CONFIG_WNM=y               # 802.11v Wireless Network Management
```

Verify on the target:

```sh
wpa_supplicant -v
# Should show: wpa_supplicant v2.10 or later
```

### Using wpa_supplicant with NetworkManager

NetworkManager uses wpa_supplicant internally but does not expose all roaming
settings. Two approaches:

**Approach A: Raw wpa_supplicant (recommended for maximum control)**

Disable NetworkManager's management of the WiFi interface and run
wpa_supplicant directly:

```sh
# Tell NetworkManager to ignore wlan0
cat > /etc/NetworkManager/conf.d/unmanaged-wifi.conf << 'EOF'
[keyfile]
unmanaged-devices=interface-name:wlan0
EOF

# Enable wpa_supplicant service for wlan0
systemctl enable wpa_supplicant@wlan0
systemctl start wpa_supplicant@wlan0

# Use dhclient or systemd-networkd for IP on wlan0
```

**Approach B: NetworkManager with supplementary wpa_supplicant config**

Create a NetworkManager connection with 802.11r:

```sh
nmcli connection add type wifi ifname wlan0 con-name packhouse \
    ssid PackhouseNet \
    wifi-sec.key-mgmt wpa-psk \
    wifi-sec.psk "your-password-here"

# Enable FT (requires NM 1.42+ on Debian 12)
nmcli connection modify packhouse 802-11-wireless-security.pmf 1
```

Note: NetworkManager does not currently expose bgscan configuration. If using
NetworkManager, roaming relies on the driver's internal roaming and 802.11v
BSS transition requests from the AP.

## Ubiquiti UniFi AP Configuration

### SSID settings

Configure the packhouse SSID in the UniFi controller:

| Setting | Value | Notes |
|---------|-------|-------|
| Security | WPA2/WPA3 Personal | WPA2 for compatibility, WPA3 optional |
| PMF (802.11w) | Optional | Must match `ieee80211w=1` in wpa_supplicant |
| 802.11k | **Enabled** | Low risk, provides neighbor reports to clients |
| 802.11v | **Enabled** | Low risk, AP can suggest roaming targets |
| 802.11r | **Disabled initially** | Enable after testing (see phased rollout below) |
| Band steering | Prefer 5 GHz | 5 GHz has less interference from packhouse machinery |
| Min-RSSI | -75 dBm | Soft-kick threshold for sticky clients |

### Channel planning for packhouses

Packhouses have metal structures, cold rooms, and conveyor systems that create
challenging RF environments:

- Use **5 GHz band** as primary — less interference from motors and equipment
- Set channels manually — disable auto-channel to prevent APs from all
  choosing the same channel
- Use **non-overlapping 80 MHz channels**: 36, 52, 100, 116 (subject to DFS
  in ZA)
- Set transmit power to **Medium** — too high causes clients to hear distant
  APs they cannot reliably reach
- Place APs every 15-20m in open areas, more densely near cold rooms and
  metal shelving

### Min-RSSI tuning

The Min-RSSI setting (-75 dBm recommended starting point) is a "soft kick":
when a client's signal drops below this level, the AP sends a disassociation
frame encouraging the client to find a better AP.

The **client bgscan threshold (-65 dBm) must be higher** (stronger signal)
than the AP Min-RSSI (-75 dBm). This creates a two-stage roaming strategy:

```
Signal strength timeline as client moves away from AP:

  -50 dBm  ███████████████████  Excellent — scan every 120s
  -60 dBm  ██████████████████
  -65 dBm  █████████████████    ← Client starts scanning every 10s
  -70 dBm  ████████████████       Client finds better AP, roams voluntarily
  -75 dBm  ███████████████      ← AP kicks client if still connected
  -80 dBm  ██████████████         Connection unreliable
  -85 dBm  █████████████          Connection lost
```

## Phased Rollout

### Phase 1: Baseline (deploy first)

- wpa_supplicant: `key_mgmt=WPA-PSK` with `proactive_key_caching=1`
- bgscan: `simple:10:-65:120`
- UniFi: 802.11k and 802.11v **enabled**, 802.11r **disabled**
- Min-RSSI: -75 dBm
- WiFi power save: disabled
- Monitor roaming behaviour with `wpa_cli` and the debug WebSocket server

This gives you proactive scanning and AP-assisted roaming without the risk
of 802.11r incompatibilities.

### Phase 2: Enable Fast Transition

After Phase 1 is stable in the field:

- wpa_supplicant: change to `key_mgmt=FT-PSK WPA-PSK`
- UniFi: enable 802.11r (Fast Roaming) on the SSID
- Test roaming across the packhouse with `wpa_cli` monitoring
- Verify handoff times are ~50ms instead of ~400ms
- If disconnections occur, revert to Phase 1

### Phase 3: Advanced tuning

- Switch bgscan from `simple` to `learn` with a persistent database:
  ```
  bgscan="learn:10:-65:120:/var/lib/wpa_supplicant/bgscan.db"
  ```
  This learns which channels the network uses and only scans those channels,
  reducing scan overhead.
- Tune bgscan thresholds per-site based on AP density
- Consider separate 2.4 GHz SSID as fallback for areas with poor 5 GHz
  penetration (cold rooms)

## Roaming Monitor Service

A lightweight systemd service can monitor roaming events and provide
diagnostics. This runs on both platforms and feeds the browser's
`WpaController` with signal data.

### /usr/local/bin/roaming-monitor.sh

```sh
#!/bin/sh
# Monitor WiFi roaming events via wpa_cli
# Logs roaming events to journal for diagnostics

INTERFACE="${1:-wlan0}"

log() {
    logger -t roaming-monitor "$@"
}

log "Starting roaming monitor on $INTERFACE"

# Monitor wpa_supplicant events
wpa_cli -i "$INTERFACE" -a /usr/local/bin/roaming-event.sh
```

### /usr/local/bin/roaming-event.sh

```sh
#!/bin/sh
# Called by wpa_cli action mode for each event

INTERFACE="$1"
EVENT="$2"

case "$EVENT" in
    CTRL-EVENT-CONNECTED)
        BSSID=$(wpa_cli -i "$INTERFACE" status | grep ^bssid= | cut -d= -f2)
        FREQ=$(wpa_cli -i "$INTERFACE" status | grep ^freq= | cut -d= -f2)
        RSSI=$(wpa_cli -i "$INTERFACE" signal_poll | grep ^RSSI= | cut -d= -f2)
        logger -t roaming-monitor "CONNECTED bssid=$BSSID freq=$FREQ rssi=$RSSI"
        ;;
    CTRL-EVENT-DISCONNECTED)
        logger -t roaming-monitor "DISCONNECTED reason=$3"
        ;;
    CTRL-EVENT-SCAN-RESULTS)
        # Count available APs on our SSID
        COUNT=$(wpa_cli -i "$INTERFACE" scan_results | grep -c "PackhouseNet")
        logger -t roaming-monitor "SCAN found $COUNT APs"
        ;;
esac
```

### /etc/systemd/system/roaming-monitor.service

```ini
[Unit]
Description=WiFi Roaming Monitor
After=wpa_supplicant.service network-online.target
Wants=network-online.target

[Service]
Type=simple
ExecStart=/usr/local/bin/roaming-monitor.sh wlan0
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### Viewing roaming events

```sh
# Live monitoring
journalctl -t roaming-monitor -f

# Recent roaming history
journalctl -t roaming-monitor --since "1 hour ago"
```

## Browser Integration

The browser's `WpaController` (currently stubbed) will expose roaming-related
data to the QML UI via D-Bus queries to wpa_supplicant or NetworkManager:

| QML property | Source | Display |
|--------------|--------|---------|
| `signalLevel` (0-4) | RSSI from `wpa_cli signal_poll` | WiFi icon in bottom bar |
| `connected` (bool) | wpa_supplicant state | Icon visibility |
| `ssid` (string) | Current SSID | Info popup |

RSSI to signal level mapping:

| RSSI | Level | Icon |
|------|-------|------|
| >= -50 dBm | 4 | Full signal |
| >= -60 dBm | 3 | Good |
| >= -67 dBm | 2 | Fair |
| >= -75 dBm | 1 | Weak |
| < -75 dBm  | 0 | Very weak / disconnected |

The browser does not participate in roaming decisions — that is handled
entirely by wpa_supplicant. The browser only displays the current state.

## Diagnostics Commands

Useful commands for debugging roaming on the target device:

```sh
# Current connection status
wpa_cli -i wlan0 status

# Signal strength and noise
wpa_cli -i wlan0 signal_poll

# Scan for nearby APs
wpa_cli -i wlan0 scan && sleep 2 && wpa_cli -i wlan0 scan_results

# Force a reassociation (trigger roam)
wpa_cli -i wlan0 reassociate

# Check if 802.11r is active
wpa_cli -i wlan0 status | grep key_mgmt
# Should show FT-PSK if Fast Transition is working

# Driver/firmware info
ethtool -i wlan0

# Check power management state
iw dev wlan0 get power_save

# Continuous RSSI monitoring
watch -n 1 'wpa_cli -i wlan0 signal_poll | grep RSSI'
```

## Module Comparison

The M.2 2230 WiFi market for embedded Linux is niche — most M.2 WiFi
modules use PCIe (which the BBB lacks). The realistic options for
non-PCIe interfaces are SDIO and USB, covering three chipset families:
NXP, Broadcom, and Realtek.

### M.2 Key E Interface Overview

The M.2 2230 Key E connector carries PCIe x1, USB 2.0, and SDIO signals.
Which interface the WiFi chip uses depends on the module — the host board
must wire the corresponding lanes:

| Interface | BBB | CM4 | Notes |
|-----------|-----|-----|-------|
| SDIO | Via expansion header | CM4 Lite only | Both platforms have SDIO |
| USB 2.0 | Yes (USB host port) | Yes (USB host port) | Universal, any host |
| PCIe x1 | **No** | Yes (via HAT) | BBB has no PCIe |

### Available M.2 2230 SDIO modules

| Module | Chipset | WiFi | MIMO | 802.11r/k/v | Price (est.) | Source |
|--------|---------|------|------|-------------|-------------|--------|
| **Ezurio ST60-2230C** (current) | NXP 88W8997 | WiFi 5 (ac) | 2x2 | Yes (proven, NXP AN14212) | ~$38 | DigiKey, Arrow |
| **Ezurio IF573-2230C** | NXP IW612 | WiFi 6 (ax) | 2x2 | Yes | ~$40+ | DigiKey |
| **SparkLAN WNFB-266AXI(BT)** | Broadcom BCM43752 | WiFi 6 (ax) | 2x2 | Via FMAC patches | ~$20-25 | Techship, EmbeddedWorks |
| **SparkLAN WNFS-267AXI(BT)** | Broadcom (6E) | WiFi 6E | 2x2 | Via FMAC patches | ~$25-30 | Specialty distributors |
| **Embedded Artists EAR00370** | NXP 88W8997 (Murata 1YM on carrier) | WiFi 5 (ac) | 2x2 | Yes | ~$30+ | Embedded Artists |

### Realtek SDIO modules (NOT available in M.2 2230)

Realtek SDIO chips (RTL8822CS, RTL8852BS) are cheap ($7-8) but only sold as
**stamp/LGA modules** from Chinese OEMs (Fn-Link, LB-LINK, Feasycom) via
Alibaba. No western distributor (DigiKey, Mouser, Farnell) stocks them in
M.2 2230 form factor. Realtek uses "-CE"/"-BE" suffix for PCIe (which is
what goes on M.2 cards) and "-CS"/"-BS" for SDIO (stamp modules only).

| Chip | WiFi | MIMO | Driver | 802.11r/k/v | Form factor |
|------|------|------|--------|-------------|-------------|
| RTL8822CS | WiFi 5 | 2x2 | rtw88 (mainline 6.x) | Unproven/unlikely | Stamp/LGA only |
| RTL8852BS | WiFi 6 | 2x2 | Vendor out-of-tree | Unproven/unlikely | Stamp/LGA only |
| RTL8821CS | WiFi 5 | 1x1 | rtw88 (mainline 6.x) | Unproven/unlikely | Stamp/LGA only |

Realtek Linux drivers (rtw88/rtw89) have the weakest reputation among WiFi
vendors for stability and feature completeness. 802.11r support through the
cfg80211 `update_ft_ies()` callback is not confirmed in any Realtek driver.
Reports on OpenWrt forums indicate 802.11r does not work with RTL8822CE
(PCIe variant of the same chip family).

Using Realtek SDIO would require:
1. Custom carrier board for the stamp module
2. Sourcing from Chinese OEMs with longer lead times
3. Accepting that 802.11r fast roaming likely will not work
4. Living with known driver stability issues

### Infineon/Cypress SDIO modules

| Chip | WiFi | MIMO | Driver | 802.11r/k/v | Notes |
|------|------|------|--------|-------------|-------|
| CYW43455 | WiFi 5 | **1x1** | brcmfmac | Yes (with FMAC patches) | Same chip as CM4 onboard — inadequate |
| CYW4373 | WiFi 5 | **1x1** | brcmfmac | Yes (with FMAC patches) | No M.2 2230 module available |
| CYW5557x | WiFi 6/6E | 2x2 | brcmfmac (FMAC) | Likely | Limited availability, long lead times |

The Infineon/Cypress chips either lack 2x2 MIMO (CYW43455, CYW4373) or are
not readily available in M.2 2230 form (CYW5557x).

### USB M.2 2230 modules

Very few M.2 2230 modules use USB for the WiFi radio — most use PCIe
(WiFi) + USB (Bluetooth only). The USB-WiFi options are:

| Module | Chipset | WiFi | MIMO | 802.11r/k/v | Price (est.) | Source |
|--------|---------|------|------|-------------|-------------|--------|
| **Ezurio ST60-2230C-U** | NXP 88W8997 | WiFi 5 (ac) | 2x2 | Yes (proven) | ~$38-42 | DigiKey, Arrow |
| **jjPlus WMU6203** | Realtek RTL8822BU | WiFi 5 (ac) | 2x2 | Unreliable | ~$42 | Techship |

**Ezurio ST60-2230C-U** — the "-U" suffix variant of the same ST60 module
uses USB 2.0 for WiFi instead of SDIO. Same NXP 88W8997 chipset, same
firmware, same roaming capabilities. Uses the mainline `mwifiex_usb`
driver (in-kernel since ~3.12). This works on **any USB host** — plug it
into a BBB USB port or CM4 USB without needing SDIO wiring. Same ~$38-42
price as the SDIO variant.

**jjPlus WMU6203** — uses Realtek RTL8822BU, a USB-native WiFi chip.
Requires an out-of-tree driver (`88x2bu`); the RTL8822BU is not in the
mainline kernel. 802.11r config flags exist in the driver source but are
disabled by default and reported as non-functional. Not recommended for
production roaming.

**PCIe modules do NOT fall back to USB for WiFi.** Intel AX200/AX210 and
similar PCIe-based M.2 modules expose only Bluetooth over USB. If the host
has no PCIe lane, the WiFi radio simply does not work.

The USB variant is attractive because it avoids SDIO wiring complexity —
both BBB and CM4 can connect via standard USB host ports. The trade-off
is USB 2.0 bandwidth (480 Mbps shared), but for a kiosk browser loading
web pages this is not a concern.

### Recommendation

**Best value: SparkLAN WNFB-266AXI(BT)** (Broadcom BCM43752)

- WiFi 6, 2x2 MIMO, SDIO, M.2 2230 — ticks all the boxes
- ~40% cheaper than the Ezurio ST60-2230C
- Uses the `brcmfmac` driver which is mature and well-maintained in mainline
- 802.11r/k/v requires Infineon's FMAC driver patches (not vanilla mainline)
  but these are well-documented and actively maintained
- Available from Techship (EU) and EmbeddedWorks (US)
- Not on DigiKey/Mouser — requires specialty embedded distributors

**Keep current: Ezurio ST60-2230C** (NXP 88W8997)

- Proven 802.11r/k/v with documented NXP application note
- Available from DigiKey and Arrow (easier procurement)
- Higher cost (~$38/unit) but lowest integration risk
- Well-tested on ARM SDIO platforms
- USB variant (ST60-2230C-U) available at same price — simplifies wiring
  by connecting via USB host port instead of SDIO, uses `mwifiex_usb`
  driver, same firmware and roaming capabilities

**Future upgrade: Ezurio IF573-2230C** (NXP IW612)

- Pin-compatible WiFi 6 successor to the ST60-2230C
- Same NXP driver family, minimal software changes
- Similar pricing to ST60

**Avoid: Realtek SDIO**

- No M.2 2230 modules exist through authorized channels
- 802.11r support unproven in Linux drivers
- Driver stability concerns
- Would require custom carrier board design

## References

- NXP AN14212: 802.11kvr Roaming Application Note
  https://www.nxp.com/docs/en/application-note/AN14212.pdf
- Ezurio ST60-2230C Product Page
  https://www.ezurio.com/wireless-modules/wifi-modules-bluetooth/60-2230c-series
- Ezurio 802.11k/v FAQ
  https://www.ezurio.com/support/faqs/how-to-support-802-11k-and-802-11v-in-st60-su60
- NXP mwifiex Out-of-Tree Driver
  https://github.com/nxp-imx/mwifiex
- Ezurio Sterling-60 Release Packages (firmware + driver)
  https://github.com/Ezurio/Sterling-60-Release-Packages/releases
- SparkLAN WNFB-266AXI(BT) Product Page
  https://www.sparklan.com/product/wnfb-266axibt-broadcom-wifi6-industrial-sdio-m-2-module/
- SparkLAN M.2 SDIO Product Line
  https://www.sparklan.com/m-2-sdio-first-product-line/
- Embedded Artists 1YM M.2 Module (Murata/NXP 88W8997)
  https://www.embeddedartists.com/products/1ym-m-2-module/
- Infineon FMAC Driver Patches for 802.11r
  https://community.infineon.com/t5/Wi-Fi-Combo/11R-Fast-Roaming-support-on-CYW43455/td-p/121621
- CM4 BCM43455 802.11r Firmware Bug
  https://bugs.launchpad.net/raspbian/+bug/1929746
- Ubiquiti UniFi Min-RSSI Documentation
  https://help.ui.com/hc/en-us/articles/221321728
- wpa_supplicant Configuration Reference
  https://w1.fi/cgit/hostap/plain/wpa_supplicant/wpa_supplicant.conf
