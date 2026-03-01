#ifndef UNIXSIGNALNOTIFIER_H
#define UNIXSIGNALNOTIFIER_H

#include <QObject>
#include <QSocketNotifier>
#include <signal.h>

class UnixSignalNotifier : public QObject
{
    Q_OBJECT
public:
    static UnixSignalNotifier *instance();
    bool installSignalHandler(int signalNumber);

signals:
    void unixSignal(int signalNumber);

private slots:
    void _socketHandler(int pipeFD);

private:
    explicit UnixSignalNotifier(QObject *parent = 0);
    ~UnixSignalNotifier();
    static void _signalHandler(int signalNumber);

    static int readPipes[_NSIG];
    static int writePipes[_NSIG];
    static QSocketNotifier *notifiers[_NSIG];
};

#endif // UNIXSIGNALNOTIFIER_H
