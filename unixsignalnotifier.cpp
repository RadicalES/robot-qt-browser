#include <unistd.h>
#include <sys/socket.h>
#include "unixsignalnotifier.h"

int UnixSignalNotifier::readPipes[_NSIG] = {};
int UnixSignalNotifier::writePipes[_NSIG];
QSocketNotifier *UnixSignalNotifier::notifiers[_NSIG];

UnixSignalNotifier::UnixSignalNotifier(QObject *parent) :
    QObject(parent)
{

}

UnixSignalNotifier::~UnixSignalNotifier()
{
    for (int i = 0; i < _NSIG; i++) {
        if (notifiers[i] != NULL) {
            delete notifiers[i];
            notifiers[i] = NULL;
            close(readPipes[i]);
            close(writePipes[i]);
            readPipes[i] = writePipes[i] = 0;
        }
    }
}

UnixSignalNotifier *UnixSignalNotifier::instance()
{
    static UnixSignalNotifier *inst = new UnixSignalNotifier();
    return inst;
}

bool UnixSignalNotifier::installSignalHandler(int signalNumber)
{
    Q_ASSERT(1 <= signalNumber && signalNumber < _NSIG);
    Q_ASSERT(readPipes[signalNumber] == 0);
    Q_ASSERT(writePipes[signalNumber] == 0);
    Q_ASSERT(notifiers[signalNumber] == NULL);

    struct sigaction sigact;

    sigact.sa_handler = UnixSignalNotifier::_signalHandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_flags |= SA_RESTART;

    if (sigaction(signalNumber, &sigact, 0)) {
        qFatal("%s: Couldn't register signal handler", Q_FUNC_INFO);
    }

    int sockets[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets)) {
        qFatal("%s: Couldn't create socketpair", Q_FUNC_INFO);
    }
    writePipes[signalNumber] = sockets[0];
    readPipes[signalNumber] = sockets[1];

    notifiers[signalNumber] = new QSocketNotifier(readPipes[signalNumber], QSocketNotifier::Read, 0);
    connect(notifiers[signalNumber], SIGNAL(activated(int)), this, SLOT(_socketHandler(int)));

    return true;
}

void UnixSignalNotifier::_socketHandler(int pipeFd)
{
    int signalNumber = -1;
    for (int i = 1; i < _NSIG; i++) {
        if (readPipes[i] == pipeFd) signalNumber = i;
    }
    if (signalNumber >= _NSIG) {
        qWarning("%s: Unable to find signal number for socket fd %d", Q_FUNC_INFO, pipeFd);
        return;
    }
    notifiers[signalNumber]->setEnabled(false);

    char dummy;
    ::read(readPipes[signalNumber], &dummy, sizeof(dummy));
    emit unixSignal(signalNumber);

    notifiers[signalNumber]->setEnabled(true);
}

void UnixSignalNotifier::_signalHandler(int signalNumber)
{
    if (writePipes[signalNumber] != 0) {
        char dummy = 1;
        ::write(writePipes[signalNumber], &dummy, sizeof(dummy));
    }
}

