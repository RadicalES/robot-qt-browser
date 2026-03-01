#ifndef WPACONTROLLER_H
#define WPACONTROLLER_H

#include <QObject>
#include <QTimer>

class WpaController : public QObject {
    Q_OBJECT
    Q_PROPERTY(int signalLevel READ signalLevel NOTIFY signalLevelChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString ssid READ ssid NOTIFY ssidChanged)

public:
    explicit WpaController(QObject* parent = nullptr);

    int signalLevel() const { return m_signalLevel; }
    bool connected() const { return m_connected; }
    QString ssid() const { return m_ssid; }

    Q_INVOKABLE void restartWifi();

signals:
    void signalLevelChanged();
    void connectedChanged();
    void ssidChanged();

private:
    // TODO: implement NetworkManager D-Bus polling
    int m_signalLevel;
    bool m_connected;
    QString m_ssid;
};

#endif
