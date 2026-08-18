#ifndef SCADAINDICATOR_H
#define SCADAINDICATOR_H

#include <QToolButton>
#include <QTimer>
#include <QFile>
#include <QPixmap>
#include <QPainter>
#include <QDateTime>
#include <QDir>

#include "robothead.h"

// SCADA server status in the kiosk toolbar.
//
// robot-scada-client publishes its state as plain files under /run/robot —
// status, last_ok, station, serverURL — which is also what the desktop tray
// indicator reads. Reading the same files keeps one source of truth and costs
// nothing: no D-Bus, no socket, no dependency on the client package being
// installed at all. A terminal without it simply shows grey.
class ScadaIndicator : public QToolButton {
    Q_OBJECT

public:
    enum State { Offline, Unprovisioned, Online };

    explicit ScadaIndicator(QWidget* parent = nullptr)
        : QToolButton(parent)
        , m_state(Offline)
    {
        setIconSize(QSize(48, 48));
        setAutoRaise(true);
        setFocusPolicy(Qt::NoFocus);   // the toolbar never takes keyboard focus
        refresh();

        // Polling beats watching: the files are tiny, tmpfs-backed, and rewritten
        // in place, which QFileSystemWatcher handles unreliably.
        connect(&m_timer, &QTimer::timeout, this, &ScadaIndicator::refresh);
        m_timer.start(5000);
    }

    State state() const { return m_state; }
    RobotHead::Variant variant() const { return variantFor(m_state); }
    QString station() const { return read("station"); }

    // The MAC the server knows this terminal by. robot-scada-client keys
    // provisioning on the first non-loopback interface in name order, which on
    // these terminals is the wired one; that is asked for explicitly here, so a
    // terminal that gains an interface sorting ahead of eth0 still reports the
    // MAC provisioning actually used. The client's own rule is the fallback,
    // for a terminal with no wired port at all.
    QString macAddress() const
    {
        const QStringList ifaces = QDir("/sys/class/net").entryList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System, QDir::Name);

        for (const QString& iface : ifaces) {
            if (iface.startsWith("eth") || iface.startsWith("en")) {
                const QString mac = readMac(iface);
                if (!mac.isEmpty())
                    return mac;
            }
        }
        for (const QString& iface : ifaces) {
            if (iface == "lo")
                continue;
            const QString mac = readMac(iface);
            if (!mac.isEmpty())
                return mac;
        }
        return QString();
    }
    QString serverUrl() const { return read("serverURL"); }
    QString statusText() const { return m_statusText; }

private slots:
    void refresh()
    {
        const QString status = read("status");
        const qint64 lastOk = read("last_ok").toLongLong();
        const qint64 age = lastOk > 0
            ? QDateTime::currentSecsSinceEpoch() - lastOk
            : -1;

        State state;
        QString text;
        if (status.isEmpty()) {
            // No /run/robot at all: the client is not installed or not running.
            state = Offline;
            text = "service not running";
        } else if (status == "ONLINE" && age >= 0 && age <= kStaleSeconds) {
            state = Online;
            text = "connected";
        } else if (status == "ONLINE") {
            // Published ONLINE but nothing heard recently — treat as no server
            // rather than healthy, so a dead link cannot read as working.
            state = Offline;
            text = "connection lost";
        } else if (status == "UNPROVISIONED") {
            state = Unprovisioned;
            text = "not provisioned";
        } else {
            state = Offline;
            text = "offline";
        }

        if (state != m_state || text != m_statusText) {
            m_state = state;
            m_statusText = text;
            setIcon(QIcon(RobotHead::pixmap(48, variantFor(state))));
            const QString site = station();
            setToolTip("SCADA server: " + text + (site.isEmpty() ? "" : " — " + site));
            emit stateChanged();
        }
    }

signals:
    void stateChanged();

private:
    // Matches the tray indicator: three ping intervals without contact is stale.
    static const int kStaleSeconds = 90;

    static QString read(const QString& name)
    {
        QFile file("/run/robot/" + name);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString();
        return QString::fromUtf8(file.readAll()).trimmed();
    }

    static QString readMac(const QString& iface)
    {
        QFile file("/sys/class/net/" + iface + "/address");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString();
        const QString mac = QString::fromUtf8(file.readAll()).trimmed().toUpper();
        return mac == "00:00:00:00:00:00" ? QString() : mac;
    }

    static RobotHead::Variant variantFor(State state)
    {
        switch (state) {
        case Online:        return RobotHead::Ok;
        case Unprovisioned: return RobotHead::Warn;
        default:            return RobotHead::Off;
        }
    }

    State m_state;
    QString m_statusText;
    QTimer m_timer;
};

#endif
