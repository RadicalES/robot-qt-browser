#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <QObject>

class SystemController : public QObject {
    Q_OBJECT

public:
    explicit SystemController(QObject* parent = nullptr);

    Q_INVOKABLE void reboot();
    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE QString systemInfo() const;
    Q_INVOKABLE bool networkAvailable() const;
};

#endif
