#ifndef IPCONFIG_H
#define IPCONFIG_H

#include <QString>

// IPv4 settings for one network device, shared by WiFi and LAN.
//
// The kiosk needs more than "join a network": a terminal on a packhouse
// network is often given a fixed address. Both link types configure the same
// way in NetworkManager — the ipv4 section of a connection profile — so this
// is deliberately device-agnostic and takes a device object path.
struct Ipv4Config {
    bool automatic = true;      // DHCP when true, manual address when false
    QString address;
    int prefix = 24;            // /24 etc; the UI shows a netmask
    QString gateway;
    QString dns;

    bool operator==(const Ipv4Config& other) const
    {
        return automatic == other.automatic && address == other.address
            && prefix == other.prefix && gateway == other.gateway
            && dns == other.dns;
    }
};

namespace NetworkIp {

// What the device is actually running with right now. Reads the applied
// IP4Config for the addresses, and the active profile for whether that came
// from DHCP or from a static setting.
Ipv4Config readActive(const QString& devicePath);

// Write the settings into the device's active connection profile and re-apply
// them. Returns false and sets error on failure — most often permission, which
// is what the polkit rule in the rootfs overlay covers.
bool apply(const QString& devicePath, const Ipv4Config& config, QString* error);

// Whether a device currently has an active connection to configure at all.
bool hasActiveConnection(const QString& devicePath);

QString prefixToNetmask(int prefix);
int netmaskToPrefix(const QString& netmask);   // -1 if not a valid mask

}

#endif
