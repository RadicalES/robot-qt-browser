#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QDBusObjectPath>

class NetworkController : public QObject {
    Q_OBJECT
    Q_PROPERTY(int signalLevel READ signalLevel NOTIFY signalLevelChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString ssid READ ssid NOTIFY ssidChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress NOTIFY ipAddressChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    // LAN. The kiosk shows wired and wireless side by side in the toolbar, and
    // both configure their IP settings through the same dialog.
    Q_PROPERTY(bool lanAvailable READ lanAvailable NOTIFY lanChanged)
    Q_PROPERTY(bool lanConnected READ lanConnected NOTIFY lanChanged)
    Q_PROPERTY(bool lanCarrier READ lanCarrier NOTIFY lanChanged)
    Q_PROPERTY(QString lanIpAddress READ lanIpAddress NOTIFY lanChanged)
    // True while the provisioning hotspot is up rather than a real uplink.
    Q_PROPERTY(bool hotspotActive READ hotspotActive NOTIFY connectedChanged)

public:
    explicit NetworkController(QObject* parent = nullptr);

    int signalLevel() const { return m_signalLevel; }
    bool connected() const { return m_connected; }
    QString ssid() const { return m_ssid; }
    QString ipAddress() const { return m_ipAddress; }
    bool scanning() const { return m_scanning; }
    QVariantList networks() const { return m_networks; }
    QString error() const { return m_error; }

    bool lanAvailable() const { return m_lanAvailable; }
    bool lanConnected() const { return m_lanConnected; }
    bool lanCarrier() const { return m_lanCarrier; }
    QString lanIpAddress() const { return m_lanIpAddress; }
    bool hotspotActive() const { return m_hotspotActive; }

    // Device object paths, for the shared IPv4 settings dialog.
    QString wifiDevicePath() const { return m_wifiDevicePath; }
    QString lanDevicePath() const { return m_lanDevicePath; }

    Q_INVOKABLE void scan();
    Q_INVOKABLE void connectToNetwork(const QString& ssid, const QString& password);
    Q_INVOKABLE void disconnectWifi();
    Q_INVOKABLE void forgetNetwork(const QString& ssid);
    Q_INVOKABLE void restartWifi();

signals:
    void signalLevelChanged();
    void connectedChanged();
    void ssidChanged();
    void ipAddressChanged();
    void scanningChanged();
    void networksChanged();
    void errorChanged();
    void lanChanged();

private slots:
    void onDevicePropertiesChanged(const QString& iface,
                                   const QVariantMap& changed,
                                   const QStringList& invalidated);
    void onAccessPointAdded(const QDBusObjectPath& apPath);
    void onAccessPointRemoved(const QDBusObjectPath& apPath);
    void pollStatus();
    void checkScanProgress();

private:
    void findWifiDevice();
    void findLanDevice();
    void pollLanStatus();
    void updateActiveConnection();
    void updateAccessPoints();
    qint64 lastScanValue();         // raw NM LastScan, CLOCK_BOOTTIME ms, -1 if never
    qint64 lastScanElapsedMs();     // ms since NM last completed a scan, -1 if never
    void finishScan();
    QVariantMap readAccessPointProperties(const QString& apPath);
    int strengthToLevel(int strength);
    QString securityString(uint flags, uint wpaFlags, uint rsnFlags);
    QString findConnectionPathForSsid(const QString& ssid);
    void setError(const QString& msg);

    QString m_wifiDevicePath;
    int m_signalLevel;
    bool m_connected;
    QString m_ssid;
    QString m_ipAddress;
    bool m_scanning;
    qint64 m_scanRequestedAt;       // LastScan when we asked, to spot completion
    int m_scanWaitElapsed;
    QTimer m_scanWaitTimer;

    bool m_lanAvailable = false;
    bool m_lanConnected = false;
    bool m_lanCarrier = false;
    QString m_lanDevicePath;
    QString m_lanIpAddress;
    bool m_hotspotActive = false;
    QVariantList m_networks;
    QString m_error;
    QTimer m_pollTimer;
    bool m_available;  // true if NetworkManager + WiFi device found
};

#endif
