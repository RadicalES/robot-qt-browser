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

public:
    explicit NetworkController(QObject* parent = nullptr);

    int signalLevel() const { return m_signalLevel; }
    bool connected() const { return m_connected; }
    QString ssid() const { return m_ssid; }
    QString ipAddress() const { return m_ipAddress; }
    bool scanning() const { return m_scanning; }
    QVariantList networks() const { return m_networks; }
    QString error() const { return m_error; }

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

private slots:
    void onDevicePropertiesChanged(const QString& iface,
                                   const QVariantMap& changed,
                                   const QStringList& invalidated);
    void onAccessPointAdded(const QDBusObjectPath& apPath);
    void onAccessPointRemoved(const QDBusObjectPath& apPath);
    void pollStatus();

private:
    void findWifiDevice();
    void updateActiveConnection();
    void updateAccessPoints();
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
    QVariantList m_networks;
    QString m_error;
    QTimer m_pollTimer;
    bool m_available;  // true if NetworkManager + WiFi device found
};

#endif
