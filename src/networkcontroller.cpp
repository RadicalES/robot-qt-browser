#include "networkcontroller.h"
#include "nmdefs.h"

#include <QFile>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDebug>
#include <QUuid>
#include <algorithm>

// NM names and settings types live in nmdefs.h, shared with the IPv4 code.

static QVariant getProperty(const QString& path, const char* iface, const QString& prop)
{
    QDBusInterface props(NM_SERVICE, path, DBUS_PROPS, QDBusConnection::systemBus());
    QDBusMessage reply = props.call("Get", QString(iface), prop);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QVariant v = reply.arguments().at(0);
        QDBusVariant dbv = v.value<QDBusVariant>();
        return dbv.variant();
    }
    return QVariant();
}

static QVariantMap getAllProperties(const QString& path, const char* iface)
{
    QDBusInterface props(NM_SERVICE, path, DBUS_PROPS, QDBusConnection::systemBus());
    QDBusMessage reply = props.call("GetAll", QString(iface));
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QVariant v = reply.arguments().at(0);
        const QDBusArgument arg = v.value<QDBusArgument>();
        QVariantMap map;
        arg >> map;
        return map;
    }
    return QVariantMap();
}

NetworkController::NetworkController(QObject* parent)
    : QObject(parent)
    , m_signalLevel(-1)
    , m_connected(false)
    , m_scanning(false)
    , m_scanRequestedAt(-1)
    , m_scanWaitElapsed(0)
    , m_available(false)
{
    qDBusRegisterMetaType<NMVariantMap>();
    qDBusRegisterMetaType<NMSettingsMap>();

    findWifiDevice();
    findLanDevice();

    if (m_available) {
        // Listen for device property changes (state, active AP)
        QDBusConnection::systemBus().connect(
            NM_SERVICE, m_wifiDevicePath, DBUS_PROPS, "PropertiesChanged",
            this, SLOT(onDevicePropertiesChanged(QString,QVariantMap,QStringList)));

        // Listen for AP added/removed
        QDBusConnection::systemBus().connect(
            NM_SERVICE, m_wifiDevicePath, NM_WIRELESS, "AccessPointAdded",
            this, SLOT(onAccessPointAdded(QDBusObjectPath)));
        QDBusConnection::systemBus().connect(
            NM_SERVICE, m_wifiDevicePath, NM_WIRELESS, "AccessPointRemoved",
            this, SLOT(onAccessPointRemoved(QDBusObjectPath)));

        // Poll device state periodically. This reads D-Bus properties only —
        // it never triggers a scan, so it does not touch the radio.
        connect(&m_pollTimer, &QTimer::timeout, this, &NetworkController::pollStatus);
        m_pollTimer.start(5000);

        connect(&m_scanWaitTimer, &QTimer::timeout,
                this, &NetworkController::checkScanProgress);

        // Initial status read
        pollStatus();
        qDebug() << "NetworkController: initialized on" << m_wifiDevicePath;
    } else {
        qWarning() << "NetworkController: no WiFi device found via NetworkManager";
    }
}

void NetworkController::findWifiDevice()
{
    QDBusInterface nm(NM_SERVICE, NM_PATH, NM_IFACE, QDBusConnection::systemBus());
    if (!nm.isValid()) {
        qWarning() << "NetworkController: NetworkManager D-Bus not available";
        return;
    }

    QDBusReply<QList<QDBusObjectPath>> reply = nm.call("GetDevices");
    if (!reply.isValid()) {
        qWarning() << "NetworkController: GetDevices failed:" << reply.error().message();
        return;
    }

    for (const QDBusObjectPath& devPath : reply.value()) {
        QVariant typeVar = getProperty(devPath.path(), NM_DEVICE, "DeviceType");
        if (typeVar.toUInt() == 2) { // NM_DEVICE_TYPE_WIFI
            m_wifiDevicePath = devPath.path();
            m_available = true;
            return;
        }
    }
}

void NetworkController::findLanDevice()
{
    QDBusInterface nm(NM_SERVICE, NM_PATH, NM_IFACE, QDBusConnection::systemBus());
    if (!nm.isValid())
        return;

    QDBusReply<QList<QDBusObjectPath>> reply = nm.call("GetDevices");
    if (!reply.isValid())
        return;

    for (const QDBusObjectPath& devPath : reply.value()) {
        const uint type = getProperty(devPath.path(), NM_DEVICE, "DeviceType").toUInt();
        if (type != NM_DEVICE_TYPE_ETHERNET)
            continue;
        m_lanDevicePath = devPath.path();
        m_lanAvailable = true;

        QDBusConnection::systemBus().connect(
            NM_SERVICE, m_lanDevicePath, DBUS_PROPS, "PropertiesChanged",
            this, SLOT(onDevicePropertiesChanged(QString,QVariantMap,QStringList)));

        qDebug() << "NetworkController: LAN device" << m_lanDevicePath;
        pollLanStatus();
        return;
    }
    qDebug() << "NetworkController: no wired device";
}

void NetworkController::pollLanStatus()
{
    if (!m_lanAvailable)
        return;

    const uint state = getProperty(m_lanDevicePath, NM_DEVICE, "State").toUInt();
    const bool connected = (state == NM_DEVICE_STATE_ACTIVATED);
    // Carrier is the cable: reported even when no profile is up, which is what
    // distinguishes "unplugged" from "plugged in but not configured".
    const bool carrier = getProperty(m_lanDevicePath, NM_WIRED, "Carrier").toBool();

    QString ip;
    if (connected) {
        const QString ip4Path = getProperty(m_lanDevicePath, NM_DEVICE, "Ip4Config")
                                    .value<QDBusObjectPath>().path();
        if (!ip4Path.isEmpty() && ip4Path != "/") {
            const QVariant addressData = getProperty(ip4Path, NM_IP4CONFIG, "AddressData");
            const QDBusArgument arg = addressData.value<QDBusArgument>();
            QList<NMVariantMap> addresses;
            arg >> addresses;
            if (!addresses.isEmpty())
                ip = addresses.first().value("address").toString();
        }
    }

    if (connected != m_lanConnected || carrier != m_lanCarrier || ip != m_lanIpAddress) {
        m_lanConnected = connected;
        m_lanCarrier = carrier;
        m_lanIpAddress = ip;
        emit lanChanged();
    }
}

void NetworkController::pollStatus()
{
    pollLanStatus();

    if (!m_available) return;

    // Device state
    uint state = getProperty(m_wifiDevicePath, NM_DEVICE, "State").toUInt();
    bool wasConnected = m_connected;
    m_connected = (state == NM_DEVICE_STATE_ACTIVATED);

    // NM80211Mode 3 is AP: the radio is running the provisioning hotspot, not
    // joined to anything. Reporting that as "connected" tells an operator the
    // terminal is on the network when it has no uplink at all — on a single
    // radio the two are mutually exclusive.
    const uint mode = getProperty(m_wifiDevicePath, NM_WIRELESS, "Mode").toUInt();
    const bool hotspot = (mode == 3);
    if (hotspot != m_hotspotActive) {
        m_hotspotActive = hotspot;
        emit connectedChanged();
    }

    if (m_connected) {
        // Active AP → SSID and signal
        QVariant apVar = getProperty(m_wifiDevicePath, NM_WIRELESS, "ActiveAccessPoint");
        QDBusObjectPath apPath = apVar.value<QDBusObjectPath>();

        if (!apPath.path().isEmpty() && apPath.path() != "/") {
            QVariantMap apProps = getAllProperties(apPath.path(), NM_AP);
            QByteArray ssidBytes = apProps.value("Ssid").toByteArray();
            QString newSsid = QString::fromUtf8(ssidBytes);
            int strength = apProps.value("Strength").toInt();
            int newLevel = strengthToLevel(strength);

            if (newSsid != m_ssid) {
                m_ssid = newSsid;
                emit ssidChanged();
            }
            if (newLevel != m_signalLevel) {
                m_signalLevel = newLevel;
                emit signalLevelChanged();
            }
        }

        // IP address
        QVariant ip4Var = getProperty(m_wifiDevicePath, NM_DEVICE, "Ip4Config");
        QDBusObjectPath ip4Path = ip4Var.value<QDBusObjectPath>();
        if (!ip4Path.path().isEmpty() && ip4Path.path() != "/") {
            QVariantMap ip4Props = getAllProperties(ip4Path.path(), NM_IP4CONFIG);
            QVariant addrData = ip4Props.value("AddressData");
            const QDBusArgument arg = addrData.value<QDBusArgument>();
            QString newIp;
            arg.beginArray();
            while (!arg.atEnd()) {
                QVariantMap entry;
                arg >> entry;
                newIp = entry.value("address").toString();
                break; // take first address
            }
            arg.endArray();

            if (newIp != m_ipAddress) {
                m_ipAddress = newIp;
                emit ipAddressChanged();
            }
        }
    } else {
        if (m_signalLevel != -1) {
            m_signalLevel = -1;
            emit signalLevelChanged();
        }
        if (!m_ssid.isEmpty()) {
            m_ssid.clear();
            emit ssidChanged();
        }
        if (!m_ipAddress.isEmpty()) {
            m_ipAddress.clear();
            emit ipAddressChanged();
        }
    }

    if (wasConnected != m_connected) {
        emit connectedChanged();
    }
}

// How long NM's cached scan results are considered fresh. The radio can scan
// while associated, so this is not about protecting the link — it is about not
// asking for a fresh scan every time the operator opens the WiFi dialog when NM
// already has results from seconds ago. NM throttles RequestScan itself and
// errors if asked too often.
static const qint64 kScanCacheMs = 15000;

// Give up waiting for a requested scan after this long.
static const int kScanTimeoutMs = 10000;
static const int kScanPollMs = 500;

// Milliseconds since boot, the clock NetworkManager's LastScan uses.
static qint64 bootTimeMs()
{
    QFile uptime("/proc/uptime");
    if (!uptime.open(QIODevice::ReadOnly))
        return -1;
    bool ok = false;
    const double seconds = uptime.readLine().split(' ').value(0).toDouble(&ok);
    return ok ? qint64(seconds * 1000.0) : -1;
}

// Raw LastScan: CLOCK_BOOTTIME milliseconds, or -1 if NM has never scanned.
qint64 NetworkController::lastScanValue()
{
    const QVariant value = getProperty(m_wifiDevicePath, NM_WIRELESS, "LastScan");
    if (!value.isValid())
        return -1;
    const qint64 lastScan = value.toLongLong();
    return lastScan < 0 ? -1 : lastScan;
}

qint64 NetworkController::lastScanElapsedMs()
{
    const qint64 lastScan = lastScanValue();
    const qint64 now = bootTimeMs();
    if (lastScan < 0 || now < 0)
        return -1;
    return qMax<qint64>(0, now - lastScan);
}

void NetworkController::finishScan()
{
    m_scanWaitTimer.stop();
    updateAccessPoints();
    if (m_scanning) {
        m_scanning = false;
        emit scanningChanged();
    }
}

// Called on the wait timer while a scan is outstanding.
void NetworkController::checkScanProgress()
{
    m_scanWaitElapsed += kScanPollMs;

    const qint64 current = lastScanValue();
    if (current >= 0 && current != m_scanRequestedAt) {
        qDebug() << "NetworkController: scan completed";
        finishScan();
        return;
    }
    if (m_scanWaitElapsed >= kScanTimeoutMs) {
        qDebug() << "NetworkController: scan did not report completion in"
                 << kScanTimeoutMs << "ms — showing what we have";
        finishScan();
    }
}

void NetworkController::scan()
{
    if (!m_available) {
        setError("WiFi not available");
        return;
    }

    setError("");

    // Show what NetworkManager already has immediately, so the list is
    // populated the instant the dialog opens rather than a scan round later.
    updateAccessPoints();

    const qint64 sinceLastScan = lastScanElapsedMs();
    if (sinceLastScan >= 0 && sinceLastScan < kScanCacheMs) {
        qDebug() << "NetworkController: cached scan is" << sinceLastScan
                 << "ms old, not rescanning";
        return;
    }

    m_scanning = true;
    emit scanningChanged();
    m_scanRequestedAt = lastScanValue();
    m_scanWaitElapsed = 0;

    QDBusInterface wireless(NM_SERVICE, m_wifiDevicePath, NM_WIRELESS,
                            QDBusConnection::systemBus());
    QDBusMessage reply = wireless.call("RequestScan", QVariantMap());
    if (reply.type() == QDBusMessage::ErrorMessage) {
        // Usually NM's own "scanning too frequently" throttle. Cached results
        // are already on screen, so this is not worth showing the operator.
        qDebug() << "NetworkController: scan request refused:" << reply.errorMessage();
        finishScan();
        return;
    }

    // Finish when NM reports a new LastScan rather than after a fixed delay:
    // the old blind 3s wait both truncated slow scans and dawdled after quick
    // ones.
    m_scanWaitTimer.start(kScanPollMs);
}

void NetworkController::updateAccessPoints()
{
    if (!m_available) return;

    QDBusInterface wireless(NM_SERVICE, m_wifiDevicePath, NM_WIRELESS,
                            QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> reply = wireless.call("GetAllAccessPoints");
    if (!reply.isValid()) {
        qWarning() << "NetworkController: GetAllAccessPoints failed:" << reply.error().message();
        return;
    }

    // Get list of saved connections for "saved" flag
    QDBusInterface settings(NM_SERVICE, "/org/freedesktop/NetworkManager/Settings",
                            NM_SETTINGS, QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> connsReply = settings.call("ListConnections");
    QSet<QString> savedSsids;
    if (connsReply.isValid()) {
        for (const QDBusObjectPath& connPath : connsReply.value()) {
            QDBusInterface conn(NM_SERVICE, connPath.path(), NM_SETTINGS_CONN,
                                QDBusConnection::systemBus());
            QDBusReply<NMSettingsMap> settingsReply = conn.call("GetSettings");
            if (settingsReply.isValid()) {
                NMSettingsMap s = settingsReply.value();
                if (s.contains("802-11-wireless")) {
                    QByteArray ssidBytes = s["802-11-wireless"].value("ssid").toByteArray();
                    savedSsids.insert(QString::fromUtf8(ssidBytes));
                }
            }
        }
    }

    // Build network list, dedup by SSID keeping strongest signal
    QMap<QString, QVariantMap> bestBySSID;

    for (const QDBusObjectPath& apPath : reply.value()) {
        QVariantMap apProps = readAccessPointProperties(apPath.path());
        if (apProps.isEmpty()) continue;

        QString ssid = apProps.value("ssid").toString();
        if (ssid.isEmpty()) continue;  // skip hidden networks

        int signal = apProps.value("signal").toInt();

        if (!bestBySSID.contains(ssid) || bestBySSID[ssid].value("signal").toInt() < signal) {
            QVariantMap entry;
            entry["ssid"] = ssid;
            entry["signal"] = signal;
            entry["security"] = apProps.value("security");
            entry["saved"] = savedSsids.contains(ssid);
            entry["connected"] = (m_connected && ssid == m_ssid);
            bestBySSID[ssid] = entry;
        }
    }

    // Sort by signal strength (descending)
    QVariantList networkList;
    for (const QVariantMap& entry : bestBySSID)
        networkList.append(entry);
    std::sort(networkList.begin(), networkList.end(),
              [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("signal").toInt() > b.toMap().value("signal").toInt();
    });

    m_networks = networkList;
    emit networksChanged();
}

QVariantMap NetworkController::readAccessPointProperties(const QString& apPath)
{
    QVariantMap apProps = getAllProperties(apPath, NM_AP);
    if (apProps.isEmpty()) return QVariantMap();

    QByteArray ssidBytes = apProps.value("Ssid").toByteArray();
    QString ssid = QString::fromUtf8(ssidBytes);
    int strength = apProps.value("Strength").toInt();
    uint flags = apProps.value("Flags").toUInt();
    uint wpaFlags = apProps.value("WpaFlags").toUInt();
    uint rsnFlags = apProps.value("RsnFlags").toUInt();

    QVariantMap result;
    result["ssid"] = ssid;
    result["signal"] = strength;
    result["security"] = securityString(flags, wpaFlags, rsnFlags);
    return result;
}

void NetworkController::connectToNetwork(const QString& ssid, const QString& password)
{
    if (!m_available) {
        setError("WiFi not available");
        return;
    }
    setError("");

    // Check if a saved connection exists for this SSID
    QString existingConn = findConnectionPathForSsid(ssid);
    QDBusInterface nm(NM_SERVICE, NM_PATH, NM_IFACE, QDBusConnection::systemBus());

    if (!existingConn.isEmpty() && password.isEmpty()) {
        // Re-activate existing saved connection
        QDBusMessage reply = nm.call("ActivateConnection",
                                     QVariant::fromValue(QDBusObjectPath(existingConn)),
                                     QVariant::fromValue(QDBusObjectPath(m_wifiDevicePath)),
                                     QVariant::fromValue(QDBusObjectPath("/")));
        if (reply.type() == QDBusMessage::ErrorMessage) {
            setError("Failed to connect: " + reply.errorMessage());
        }
        return;
    }

    // Find the AP object path for this SSID
    QDBusInterface wireless(NM_SERVICE, m_wifiDevicePath, NM_WIRELESS,
                            QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> apsReply = wireless.call("GetAllAccessPoints");
    QString apPath = "/";
    if (apsReply.isValid()) {
        for (const QDBusObjectPath& ap : apsReply.value()) {
            QVariant ssidVar = getProperty(ap.path(), NM_AP, "Ssid");
            if (QString::fromUtf8(ssidVar.toByteArray()) == ssid) {
                apPath = ap.path();
                break;
            }
        }
    }

    // Build connection settings
    NMSettingsMap conn;
    conn["connection"]["id"] = QVariant(ssid);
    conn["connection"]["uuid"] = QVariant(QUuid::createUuid().toString()
                                          .remove('{').remove('}'));
    conn["connection"]["type"] = QVariant(QString("802-11-wireless"));
    conn["connection"]["autoconnect"] = QVariant(true);

    conn["802-11-wireless"]["ssid"] = QVariant(ssid.toUtf8());
    conn["802-11-wireless"]["mode"] = QVariant(QString("infrastructure"));

    if (!password.isEmpty()) {
        conn["802-11-wireless-security"]["key-mgmt"] = QVariant(QString("wpa-psk"));
        conn["802-11-wireless-security"]["psk"] = QVariant(password);
    }

    conn["ipv4"]["method"] = QVariant(QString("auto"));
    conn["ipv6"]["method"] = QVariant(QString("ignore"));

    // If existing connection, delete it first so we recreate with new password
    if (!existingConn.isEmpty()) {
        QDBusInterface oldConn(NM_SERVICE, existingConn, NM_SETTINGS_CONN,
                               QDBusConnection::systemBus());
        oldConn.call("Delete");
    }

    QDBusMessage reply = nm.call("AddAndActivateConnection",
                                 QVariant::fromValue(conn),
                                 QVariant::fromValue(QDBusObjectPath(m_wifiDevicePath)),
                                 QVariant::fromValue(QDBusObjectPath(apPath)));

    if (reply.type() == QDBusMessage::ErrorMessage) {
        setError("Failed to connect: " + reply.errorMessage());
        qWarning() << "NetworkController: connect failed:" << reply.errorMessage();
    } else {
        qDebug() << "NetworkController: connecting to" << ssid;
    }
}

void NetworkController::disconnectWifi()
{
    if (!m_available) return;

    QDBusInterface dev(NM_SERVICE, m_wifiDevicePath, NM_DEVICE,
                       QDBusConnection::systemBus());
    dev.call("Disconnect");
    qDebug() << "NetworkController: disconnect requested";
}

void NetworkController::forgetNetwork(const QString& ssid)
{
    QString connPath = findConnectionPathForSsid(ssid);
    if (connPath.isEmpty()) {
        setError("No saved connection for " + ssid);
        return;
    }

    QDBusInterface conn(NM_SERVICE, connPath, NM_SETTINGS_CONN,
                        QDBusConnection::systemBus());
    QDBusMessage reply = conn.call("Delete");
    if (reply.type() == QDBusMessage::ErrorMessage) {
        setError("Failed to forget: " + reply.errorMessage());
    } else {
        qDebug() << "NetworkController: forgot" << ssid;
        // Refresh network list to update saved flags
        updateAccessPoints();
    }
}

void NetworkController::restartWifi()
{
    if (!m_available) return;

    qDebug() << "NetworkController: restarting WiFi";
    disconnectWifi();

    // Re-activate after a brief delay
    QTimer::singleShot(2000, this, [this]() {
        // Find a saved connection to re-activate
        QDBusInterface settings(NM_SERVICE, "/org/freedesktop/NetworkManager/Settings",
                                NM_SETTINGS, QDBusConnection::systemBus());
        QDBusReply<QList<QDBusObjectPath>> connsReply = settings.call("ListConnections");
        if (!connsReply.isValid()) return;

        for (const QDBusObjectPath& connPath : connsReply.value()) {
            QDBusInterface conn(NM_SERVICE, connPath.path(), NM_SETTINGS_CONN,
                                QDBusConnection::systemBus());
            QDBusReply<NMSettingsMap> s = conn.call("GetSettings");
            if (!s.isValid()) continue;

            if (s.value().contains("802-11-wireless")) {
                // Found a WiFi connection — activate it
                QDBusInterface nm(NM_SERVICE, NM_PATH, NM_IFACE,
                                  QDBusConnection::systemBus());
                nm.call("ActivateConnection",
                        QVariant::fromValue(connPath),
                        QVariant::fromValue(QDBusObjectPath(m_wifiDevicePath)),
                        QVariant::fromValue(QDBusObjectPath("/")));
                qDebug() << "NetworkController: re-activating" << connPath.path();
                return;
            }
        }
    });
}

void NetworkController::onDevicePropertiesChanged(const QString& iface,
                                                   const QVariantMap& /*changed*/,
                                                   const QStringList& /*invalidated*/)
{
    if (iface == NM_DEVICE || iface == NM_WIRELESS) {
        pollStatus();
    }
}

void NetworkController::onAccessPointAdded(const QDBusObjectPath& /*apPath*/)
{
    // Refresh network list when a new AP appears
    if (!m_scanning) {
        updateAccessPoints();
    }
}

void NetworkController::onAccessPointRemoved(const QDBusObjectPath& /*apPath*/)
{
    if (!m_scanning) {
        updateAccessPoints();
    }
}

int NetworkController::strengthToLevel(int strength)
{
    // NM gives 0-100 percentage
    if (strength >= 80) return 4;
    if (strength >= 60) return 3;
    if (strength >= 40) return 2;
    if (strength >= 20) return 1;
    return 0;
}

QString NetworkController::securityString(uint flags, uint wpaFlags, uint rsnFlags)
{
    // NM_802_11_AP_FLAGS_PRIVACY = 0x1
    if (flags == 0 && wpaFlags == 0 && rsnFlags == 0)
        return "Open";

    // RSN (WPA2/WPA3)
    if (rsnFlags & 0x400) return "WPA3";   // SAE
    if (rsnFlags & 0x200) return "WPA2-EAP";
    if (rsnFlags & 0x100) return "WPA2";   // PSK

    // WPA1
    if (wpaFlags & 0x200) return "WPA-EAP";
    if (wpaFlags & 0x100) return "WPA";

    // Privacy flag without WPA = WEP
    if (flags & 0x1) return "WEP";

    return "Secured";
}

QString NetworkController::findConnectionPathForSsid(const QString& ssid)
{
    QDBusInterface settings(NM_SERVICE, "/org/freedesktop/NetworkManager/Settings",
                            NM_SETTINGS, QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> reply = settings.call("ListConnections");
    if (!reply.isValid()) return QString();

    QByteArray targetSsid = ssid.toUtf8();
    for (const QDBusObjectPath& connPath : reply.value()) {
        QDBusInterface conn(NM_SERVICE, connPath.path(), NM_SETTINGS_CONN,
                            QDBusConnection::systemBus());
        QDBusReply<NMSettingsMap> s = conn.call("GetSettings");
        if (!s.isValid()) continue;

        NMSettingsMap settings = s.value();
        if (settings.contains("802-11-wireless")) {
            QByteArray ssidBytes = settings["802-11-wireless"].value("ssid").toByteArray();
            if (ssidBytes == targetSsid) {
                return connPath.path();
            }
        }
    }
    return QString();
}

void NetworkController::setError(const QString& msg)
{
    if (msg != m_error) {
        m_error = msg;
        emit errorChanged();
    }
}
