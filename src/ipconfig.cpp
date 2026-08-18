#include "ipconfig.h"
#include "nmdefs.h"

#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QDBusArgument>
#include <QDBusMessage>
#include <QHostAddress>
#include <QRegularExpression>
#include <QtEndian>
#include <QDebug>

namespace {

QVariant property(const QString& path, const char* iface, const char* name)
{
    QDBusInterface props(NM_SERVICE, path, DBUS_PROPS, QDBusConnection::systemBus());
    QDBusReply<QVariant> reply = props.call("Get", QString(iface), QString(name));
    return reply.isValid() ? reply.value() : QVariant();
}

// The settings profile behind a device's active connection, or empty.
QString activeProfilePath(const QString& devicePath)
{
    const QVariant activeVar = property(devicePath, NM_DEVICE, "ActiveConnection");
    const QString activePath = activeVar.value<QDBusObjectPath>().path();
    if (activePath.isEmpty() || activePath == "/")
        return QString();

    const QVariant connVar = property(activePath, NM_ACTIVE_CONN, "Connection");
    const QString connPath = connVar.value<QDBusObjectPath>().path();
    return (connPath == "/") ? QString() : connPath;
}

} // namespace

namespace NetworkIp {

QString prefixToNetmask(int prefix)
{
    if (prefix < 0 || prefix > 32)
        return QString();
    const quint32 mask = prefix == 0 ? 0 : (0xFFFFFFFFu << (32 - prefix));
    return QHostAddress(mask).toString();
}

int netmaskToPrefix(const QString& netmask)
{
    QHostAddress addr;
    if (!addr.setAddress(netmask))
        return -1;
    quint32 mask = addr.toIPv4Address();
    int prefix = 0;
    // A netmask is contiguous ones followed by contiguous zeros; anything else
    // is a typo rather than a mask.
    while (mask & 0x80000000u) {
        ++prefix;
        mask <<= 1;
    }
    return mask == 0 ? prefix : -1;
}

bool hasActiveConnection(const QString& devicePath)
{
    return !activeProfilePath(devicePath).isEmpty();
}

Ipv4Config readActive(const QString& devicePath)
{
    Ipv4Config config;
    if (devicePath.isEmpty())
        return config;

    // Applied addresses come from IP4Config, which reflects reality whether it
    // arrived by DHCP or from a static profile.
    const QString ip4Path =
        property(devicePath, NM_DEVICE, "Ip4Config").value<QDBusObjectPath>().path();
    if (!ip4Path.isEmpty() && ip4Path != "/") {
        const QVariant addressData = property(ip4Path, NM_IP4CONFIG, "AddressData");
        const QDBusArgument arg = addressData.value<QDBusArgument>();
        QList<NMVariantMap> addresses;
        arg >> addresses;
        if (!addresses.isEmpty()) {
            config.address = addresses.first().value("address").toString();
            config.prefix = addresses.first().value("prefix").toInt();
        }
        config.gateway = property(ip4Path, NM_IP4CONFIG, "Gateway").toString();

        const QVariant nsData = property(ip4Path, NM_IP4CONFIG, "NameserverData");
        const QDBusArgument nsArg = nsData.value<QDBusArgument>();
        QList<NMVariantMap> nameservers;
        nsArg >> nameservers;
        QStringList servers;
        for (const NMVariantMap& ns : nameservers)
            servers << ns.value("address").toString();
        config.dns = servers.join(", ");
    }

    // Whether that is DHCP or static comes from the profile, not the result.
    const QString profile = activeProfilePath(devicePath);
    if (!profile.isEmpty()) {
        QDBusInterface conn(NM_SERVICE, profile, NM_SETTINGS_CONN,
                            QDBusConnection::systemBus());
        QDBusReply<NMSettingsMap> settings = conn.call("GetSettings");
        if (settings.isValid()) {
            const QString method = settings.value().value("ipv4").value("method").toString();
            config.automatic = (method != "manual");
        }
    }
    return config;
}

bool apply(const QString& devicePath, const Ipv4Config& config, QString* error)
{
    const QString profile = activeProfilePath(devicePath);
    if (profile.isEmpty()) {
        if (error) *error = "No active connection to configure";
        return false;
    }

    QDBusInterface conn(NM_SERVICE, profile, NM_SETTINGS_CONN,
                        QDBusConnection::systemBus());
    QDBusReply<NMSettingsMap> current = conn.call("GetSettings");
    if (!current.isValid()) {
        if (error) *error = "Could not read the connection profile";
        return false;
    }

    NMSettingsMap settings = current.value();
    NMVariantMap ipv4;
    if (config.automatic) {
        ipv4.insert("method", "auto");
        // Leave nothing static behind, or NM keeps applying it alongside DHCP.
        ipv4.insert("address-data", QVariant::fromValue(QList<NMVariantMap>()));
        ipv4.insert("gateway", QString());
        ipv4.insert("dns", QVariant::fromValue(QList<uint>()));
        ipv4.insert("dns-search", QStringList());
    } else {
        QHostAddress addr;
        if (!addr.setAddress(config.address)) {
            if (error) *error = "Invalid IP address";
            return false;
        }
        NMVariantMap entry;
        entry.insert("address", config.address);
        entry.insert("prefix", uint(config.prefix));
        ipv4.insert("method", "manual");
        ipv4.insert("address-data", QVariant::fromValue(QList<NMVariantMap>() << entry));
        ipv4.insert("gateway", config.gateway);

        // NM wants nameservers as network-order 32-bit integers on the "dns"
        // key; the string form is only on the read side.
        QList<uint> dnsList;
        const QStringList servers = config.dns.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        for (const QString& server : servers) {
            QHostAddress dnsAddr;
            if (dnsAddr.setAddress(server))
                dnsList << qToBigEndian(dnsAddr.toIPv4Address());
        }
        ipv4.insert("dns", QVariant::fromValue(dnsList));
    }
    settings.insert("ipv4", ipv4);

    QDBusMessage update = conn.call("Update", QVariant::fromValue(settings));
    if (update.type() == QDBusMessage::ErrorMessage) {
        if (error) *error = update.errorMessage();
        return false;
    }

    // Re-activate so the change takes effect now rather than at next connect.
    QDBusInterface nm(NM_SERVICE, NM_PATH, NM_IFACE, QDBusConnection::systemBus());
    QDBusMessage activate = nm.call("ActivateConnection",
                                    QVariant::fromValue(QDBusObjectPath(profile)),
                                    QVariant::fromValue(QDBusObjectPath(devicePath)),
                                    QVariant::fromValue(QDBusObjectPath("/")));
    if (activate.type() == QDBusMessage::ErrorMessage) {
        if (error) *error = activate.errorMessage();
        return false;
    }
    return true;
}

} // namespace NetworkIp
