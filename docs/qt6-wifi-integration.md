# Qt6 native WiFi integration — RBT440

How the on-device Qt6 application can configure WiFi and read AP / signal
detail directly, instead of shelling out to `nmcli` or bouncing through
the web UI.

Everything marked **[verified]** was checked live on the reference unit
(`10.224.40.189`, kernel `6.12.25+rpt-rpi-v8`, NetworkManager 1.42.4,
Ezurio `sdcsupp`, `lrdmwl` driver) on 2026-08-17. Anything not marked was
not exercised — treat it as design intent, not proven behaviour.

---

## 1. Start here: what already exists on the device

Do not build a parallel WiFi stack. The device already has an opinionated
one, and fighting it will produce a forklift that loses its network.

| Component | Role |
|---|---|
| **NetworkManager 1.42.4** | Owns `wlan0`. All profiles, IP config, autoconnect |
| **Ezurio `sdcsupp`** (wpa_supplicant 2.10 fork) | The supplicant NM drives. Adds 802.11r/k/v and `bgscan_summit` roaming |
| **`lrdmwl`** | Driver for the ST60-2230C (Marvell 88W8997). Interface is **`wlan0`** |
| **`wifi-provisioning.service`** | On boot: if no infrastructure profile exists, raises an AP-mode hotspot `rbt440-Setup-<MAC tail>` at `10.42.0.1` and the captive portal |
| **`/usr/local/sbin/reset-wifi-credentials`** | Sanctioned "forget WiFi and re-enter setup mode" helper |
| **webapp CGIs** | `getcomms.sh`, `getstate.sh`, `scanwifi.sh`, `setprovision.sh`, `resetwifi.sh` — the web UI's WiFi surface |

**Qt6 has no WiFi API.** Bearer Management (`QNetworkConfigurationManager`)
was removed in Qt6, and there is no `QtWiFi`. So the choice is D-Bus,
the supplicant control socket, or an existing helper — nothing built in.

> **Note:** the reference unit has **no Qt6 runtime installed at all**
> **[verified]** — zero `libQt6*` libraries. Deployment must either add
> the Qt6 packages (`libqt6dbus6`, `libqt6network6`, …) to the rootfs or
> ship a self-contained bundle.

---

## 2. Recommended architecture

Three layers, each for what it is genuinely best at:

```
┌──────────────────────────────────────────────────────────┐
│  Qt6 app                                                 │
│                                                          │
│  A. NetworkManager D-Bus  (QtDBus)      ← configure+state │
│     scan, list APs, connect, disconnect, IP, signal %     │
│                                                          │
│  B. sdcsupp control socket (QLocalSocket)  ← diagnostics  │
│     dBm RSSI, roaming events, FT/BTM, current BSSID       │
│                                                          │
│  C. reset-wifi-credentials (QProcess)  ← re-provisioning  │
│     hand back to the hotspot/portal flow                  │
└──────────────────────────────────────────────────────────┘
```

- **A is the primary interface.** Typed, signal-driven, no output parsing,
  and it cooperates with the profile store the rest of the system uses.
- **B is read-mostly.** It exposes things NM does not: RSSI in dBm, live
  roaming events, which BSS you are on. NM is already attached to this
  supplicant — see the two-masters warning in §4.
- **C exists because provisioning is a system-level state machine**, not
  an app concern. Do not re-implement hotspot teardown.

Avoid parsing `nmcli` output. It is a human-facing CLI, its columns move
between releases, and you already have the same data typed over D-Bus.

---

## 3. Layer A — NetworkManager over QtDBus

### 3.1 Object layout **[verified]**

```
Service:  org.freedesktop.NetworkManager        (system bus)
Manager:  /org/freedesktop/NetworkManager
Device:   /org/freedesktop/NetworkManager/Devices/3      ← wlan0 on this unit
AP:       /org/freedesktop/NetworkManager/AccessPoint/<n>
```

Never hardcode `Devices/3` — the index changes. Resolve it:

```cpp
QDBusInterface nm("org.freedesktop.NetworkManager",
                  "/org/freedesktop/NetworkManager",
                  "org.freedesktop.NetworkManager",
                  QDBusConnection::systemBus());
QDBusReply<QDBusObjectPath> dev = nm.call("GetDeviceByIpIface", "wlan0");
```

### 3.2 Scanning **[verified]**

`org.freedesktop.NetworkManager.Device.Wireless` provides:

| Member | Sig | Notes |
|---|---|---|
| `RequestScan(a{sv})` | method | Pass an empty dict. Async — returns immediately |
| `GetAllAccessPoints()` | → `ao` | Includes APs not currently visible |
| `AccessPoints` | `ao` | Property, `emits-change`. 10 APs cached on the test unit |
| `ActiveAccessPoint` | `o` | `"/"` when not associated |
| `Bitrate` | `u` | kb/s, 0 when down |
| `LastScan` | `x` | `CLOCK_BOOTTIME` ms. **-1 means never scanned** |
| `HwAddress` / `PermHwAddress` | `s` | `B0:FB:15:21:AD:F0` (Ezurio OUI) |
| `WirelessCapabilities` | `u` | `10239` on this unit |
| `AccessPointAdded` / `AccessPointRemoved` | signals | `o` |

Rate-limit scans. NM itself throttles `RequestScan`, and on a single-radio
device every scan briefly interrupts traffic — a scan-on-timer at 5 s will
degrade the forklift's own link. Scan on user action, then read the cached
`AccessPoints` property.

### 3.3 AP properties **[verified]**

From `org.freedesktop.NetworkManager.AccessPoint`, real values from the
test unit:

| Property | Sig | Example | Meaning |
|---|---|---|---|
| `Ssid` | `ay` | `[73 79 84 45 ...]` | **Byte array, not a string.** May contain non-UTF-8 or embedded NULs |
| `HwAddress` | `s` | `76:4D:28:F9:AD:3B` | BSSID |
| `Strength` | `y` | `29` | **0–100 percent, NOT dBm.** For dBm use layer B |
| `Frequency` | `u` | `2452` | MHz → channel 9 |
| `MaxBitrate` | `u` | `270000` | kb/s → 270 Mb/s |
| `Mode` | `u` | `2` | `NM_802_11_MODE_INFRA` |
| `Flags` | `u` | `1` | `PRIVACY` |
| `WpaFlags` / `RsnFlags` | `u` | `392` | Security, see below |
| `LastSeen` | `i` | `8528` | `CLOCK_BOOTTIME` seconds; `-1` = never |

Handle `Ssid` as `QByteArray`:

```cpp
QByteArray raw = reply.value().toByteArray();
QString ssid = QString::fromUtf8(raw);   // may need fallback for odd APs
```

**Decoding the security flags.** `NM80211ApSecurityFlags` is a bitfield:

| Bit | Meaning | | Bit | Meaning |
|---|---|---|---|---|
| `0x001` | PAIR_WEP40 | | `0x080` | GROUP_CCMP |
| `0x002` | PAIR_WEP104 | | `0x100` | KEY_MGMT_PSK |
| `0x004` | PAIR_TKIP | | `0x200` | KEY_MGMT_802_1X |
| `0x008` | PAIR_CCMP | | `0x400` | KEY_MGMT_SAE |
| `0x010` | GROUP_WEP40 | | `0x800` | KEY_MGMT_OWE |
| `0x020` | GROUP_WEP104 | | `0x2000` | KEY_MGMT_EAP_SUITE_B_192 |
| `0x040` | GROUP_TKIP | | | |

So the observed `RsnFlags = 392` = `0x100 | 0x080 | 0x008` =
**KEY_MGMT_PSK + GROUP_CCMP + PAIR_CCMP** → WPA2-PSK (CCMP). That is the
computation to drive the padlock icon and to decide whether to prompt for
a passphrase, an 802.1X identity, or nothing (open/OWE).

Channel from frequency:

```cpp
int chan24 = (freq - 2407) / 5;              // 2412..2484
int chan5  = (freq - 5000) / 5;              // 5xxx
```

### 3.4 Connecting

`AddAndActivateConnection` in one call — it creates the profile and brings
it up, so there is no window where a half-made profile lingers:

```cpp
QVariantMap conn {
    {"type", "802-11-wireless"},
    {"id",   ssid},
    {"uuid", QUuid::createUuid().toString(QUuid::WithoutBraces)},
};
QVariantMap wifi {
    {"ssid", rawSsid},                    // ay, the QByteArray
    {"mode", "infrastructure"},
};
QVariantMap sec {
    {"key-mgmt", "wpa-psk"},
    {"psk",      passphrase},
};
QMap<QString, QVariantMap> settings {
    {"connection",             conn},
    {"802-11-wireless",        wifi},
    {"802-11-wireless-security", sec},
    {"ipv4", {{"method", "auto"}}},
    {"ipv6", {{"method", "ignore"}}},
};
nm.call("AddAndActivateConnection", QVariant::fromValue(settings),
        QVariant::fromValue(devPath), QVariant::fromValue(QDBusObjectPath("/")));
```

To match this product's roaming setup, mirror what the docs prescribe:
`key-mgmt` of `"wpa-psk ft-psk"` to allow 802.11r, and
`wifi.bgscan = "summit:30:-75:300"`. See
`software/bringup/st60-2230c-bringup.md` § Roaming.

Register the `QDBusMetaType` for `a{sa{sv}}` before the call, or the
marshalling silently produces an empty dict:

```cpp
qDBusRegisterMetaType<QMap<QString, QVariantMap>>();
```

### 3.5 Watching state

Subscribe rather than poll:

- `org.freedesktop.NetworkManager.Device` → `StateChanged(u new, u old, u reason)`.
  The `reason` code is what distinguishes "wrong password"
  (`NM_DEVICE_STATE_REASON_NO_SECRETS`, 7) from "AP gone"
  (`SSID_NOT_FOUND`, 53) — surface that to the operator rather than a
  generic failure.
- `org.freedesktop.DBus.Properties` → `PropertiesChanged` on the AP object
  for live `Strength`, and on the device for `Bitrate`.
- `ActiveAccessPoint` changing is your roam indicator at the NM level.

---

## 4. Layer B — the sdcsupp control socket

### 4.1 Access **[verified]**

```
srwxrwx--- 1 root netdev  /run/wpa_supplicant/wlan0
```

The socket is group `netdev`, and the `robot` user **is already in
`netdev`** (gid 110) **[verified]** — so the Qt app can talk to the
supplicant **without sudo**. Use `QLocalSocket` in unconnected/datagram
mode, or link `libwpa_client`.

`wpa_cli` is installed at `/usr/sbin/wpa_cli` but **not on the `robot`
user's `PATH`** (Debian excludes `/usr/sbin` for non-root) **[verified]** —
use the absolute path if you shell out for debugging.

### 4.2 What it gives you that NM does not

| Command | Returns |
|---|---|
| `STATUS` | `wpa_state`, `bssid`, `ssid`, `freq`, `key_mgmt`, IP — **[verified]** working |
| `SIGNAL_POLL` | **RSSI in dBm**, plus noise, average beacon RSSI, tx bitrate |
| `BSS <idx>` / `BSS <bssid>` | Full IE dump for one BSS, including RSN/MDE (802.11r mobility domain) |
| `SCAN_RESULTS` | bssid / frequency / signal(dBm) / flags / ssid table |
| `GET_CAPABILITY key_mgmt` | **[verified]**: `FT-PSK FT-EAP SAE OWE DPP FILS-SHA256 … WPA-EAP-SUITE-B-192` |
| `ATTACH` then read | Async event stream — see below |

`SIGNAL_POLL` returned `FAIL` on the test unit **[verified]** — because
`wpa_state=DISCONNECTED` with no association. It needs an active link;
this was not re-tested associated (the unit has **no antennas fitted**,
see §7).

`/proc/net/wireless` exists but had **header only, no `wlan0` row**
**[verified]**, again because the interface is down. It is a WEXT-era
interface; prefer `SIGNAL_POLL`.

### 4.3 Roaming events

After `ATTACH`, the socket emits unsolicited `<3>CTRL-EVENT-*` lines. The
ones worth surfacing on a forklift:

- `CTRL-EVENT-CONNECTED` / `CTRL-EVENT-DISCONNECTED` (carries `reason=`)
- `CTRL-EVENT-SCAN-STARTED` / `CTRL-EVENT-SCAN-RESULTS`
- `CTRL-EVENT-BEACON-LOSS` — the early warning of a dead zone
- `CTRL-EVENT-REGDOM-CHANGE`
- FT / BTM activity, which is how you prove 802.11r and 802.11v are
  actually being used rather than merely enabled

This is the data that would let the app show "roamed 4 times crossing the
cool store" — genuinely useful for the packhouse deployment, and not
obtainable from NM.

### 4.4 Two-masters warning

**NetworkManager is already attached to this supplicant.** Issuing state
-changing commands (`SELECT_NETWORK`, `ADD_NETWORK`, `DISCONNECT`,
`REASSOCIATE`) will desynchronise NM from reality: NM believes it owns the
profile set, and will fight or overwrite you.

Rule: **read-only through layer B; all changes through layer A.**
`STATUS`, `SIGNAL_POLL`, `SCAN_RESULTS`, `BSS`, `GET_CAPABILITY` and event
monitoring are safe. Anything that mutates is not.

---

## 5. Permissions — the part that will bite

**Reading NM state unprivileged works** — `robot` read `LastScan` and the
AP properties fine **[verified]**.

**Writing does not, at least over SSH.** `pkcheck --action-id
org.freedesktop.NetworkManager.settings.modify.system` for the `robot`
user returned **`Not authorized`** **[verified]**.

Important nuance: polkit distinguishes an *active local session* from a
remote one, and NM's stock rules grant much of `network-control` to
`allow_active` only. The check above ran over SSH, which is **not** an
active session — so it may well succeed from the app's own seat-0 session.
**Re-test from inside the running Qt app before designing around it:**

```cpp
// or from a terminal in the graphical session:
// pkcheck --action-id org.freedesktop.NetworkManager.settings.modify.system --process $$
```

If it is denied there too, pick one:

**Option 1 — a polkit rule (preferred).** Ship
`/etc/polkit-1/rules.d/50-rbt440-network.rules`:

```javascript
// Let the kiosk user configure WiFi without sudo.
polkit.addRule(function(action, subject) {
    if (subject.user == "robot" &&
        (action.id == "org.freedesktop.NetworkManager.settings.modify.system" ||
         action.id == "org.freedesktop.NetworkManager.network-control" ||
         action.id == "org.freedesktop.NetworkManager.enable-disable-wifi" ||
         action.id == "org.freedesktop.NetworkManager.wifi.scan")) {
        return polkit.Result.YES;
    }
});
```

Narrow, auditable, and it survives NM upgrades. This belongs in the
**rootfs** overlay (`robot-t440-rootfs`), not in the WiFi package.

**Option 2 — sudo the existing helpers.** `/etc/sudoers.d/www-nmcli`
already grants `www-data` blanket `nmcli` plus specific `systemctl` and
`reset-wifi-credentials` **[verified]**. An equivalent fragment for
`robot` would work, but it means going back to CLI parsing — the thing
§2 advises against.

`/etc/sudoers.d/wifi-provisioning` already permits
`robot ALL=(root) NOPASSWD: /usr/local/sbin/reset-wifi-credentials`
**[verified]** — so layer C needs no new rules.

---

## 6. Layer C — re-provisioning from the app

The operator-facing "forget WiFi / re-run setup" action must **not** just
delete a profile. Call the helper:

```cpp
QProcess::execute("/usr/bin/sudo",
                  {"-n", "/usr/local/sbin/reset-wifi-credentials"});
```

That stops `wifi-provisioning`, tears down and deletes the `rbt440-setup`
hotspot, deletes every infrastructure profile, and restarts provisioning —
so the unit comes back broadcasting `rbt440-Setup-<MAC tail>` with the
captive portal live. Takes ~2 s, no reboot.

**The app is in the best position to fix a known gap.** Today
`wifi-provisioning.service` only asks *"does an infrastructure profile
exist?"* — not *"are we actually connected?"*. If the AP is
decommissioned or the passphrase rotated, the device retries forever and
never re-opens the portal; recovery needs a physical SD-card marker
(`/boot/firmware/reset-wifi`). A Qt app that already watches
`StateChanged` could offer "can't reach the network — re-run setup?" after
a grace period, turning a site visit into a screen tap.

---

## 7. Product-specific gotchas

- **Single radio.** AP mode (provisioning hotspot) and station mode are
  mutually exclusive. While the hotspot is up there is no uplink. Do not
  offer a "scan" button in provisioning mode without accounting for the
  hotspot dropping.
- **AP mode needs `hostapd`.** `sdcsupp` is built without `CONFIG_AP`, so
  NM delegates AP mode to `hostapd` — which is a hard dependency of the
  `rbt440-wifi` package and kept masked so NM drives it. The app should
  never start `hostapd` itself.
- **No antennas fitted** on the reference module. Any RSSI or link-budget
  number read today is meaningless for production. Build the UI, but do
  not tune thresholds against current readings.
- **Interface is `wlan0`**, not `mlan0`. `mlan0` is the mainline `mwifiex`
  convention; `lrdmwl` gives `wlan0` **[verified]**.
- **Regulatory domain** comes from `cfg80211.ieee80211_regdom=ZA` on the
  kernel command line. Channel availability (especially 5 GHz DFS) follows
  from it — if the app shows a channel list, it must reflect the regdom,
  not a hardcoded table.
- **`iw` and `iwconfig` are not installed** **[verified]**. Don't depend on
  them; if you want `iw`-style station statistics, either add the package
  to the rootfs or use `SIGNAL_POLL`.
- **Kernel pinning.** The driver is an out-of-tree module ABI-bound to
  `6.12.25+rpt-rpi-v8`. Irrelevant to app code, but it means the app must
  not assume it can trigger OS updates.

---

## 8. Suggested build order

1. **Read-only status panel.** Resolve the device, read `ActiveAccessPoint`
   → SSID / BSSID / `Strength` / `Frequency` / `Bitrate`, subscribe to
   `PropertiesChanged`. No new permissions needed — this works today.
2. **Scan + AP list.** `RequestScan` on user action, render `AccessPoints`
   with decoded security flags and channel. Still read-only.
3. **Resolve the polkit question** (§5) from inside the app's session, then
   implement connect via `AddAndActivateConnection`, with `StateChanged`
   reason codes driving the error messages.
4. **Diagnostics tab.** Attach to the supplicant socket for dBm RSSI and
   roaming/beacon-loss events. Read-only, no sudo (`netdev` covers it).
5. **Re-provisioning.** Wire the reset helper, and add the
   connectivity-grace-period recovery from §6.

Steps 1, 2 and 4 need no privilege changes at all, which makes them a
sensible first slice.

---

## 9. Reference

- `software/wireless-features.md` — radio, security, roaming, regulatory
  capabilities of the product
- `software/bringup/st60-2230c-bringup.md` § Roaming — FT / bgscan_summit
  / `summit_flags` knobs and the `nmcli` equivalents
- `software/wifi-provisioning/README.md` — the provisioning state machine
  and all three reset paths
- `design-notes/cm4-gpio-mapping.md` — SDIO pin map (BCM22-27)
- NetworkManager D-Bus API: `https://networkmanager.dev/docs/api/latest/`
- wpa_supplicant control interface:
  `https://w1.fi/wpa_supplicant/devel/ctrl_iface_page.html`
