#include "systemcontroller.h"
#include <QCoreApplication>
#include <QProcess>
#include <QSysInfo>
#include <QNetworkInterface>
#include <QDebug>

SystemController::SystemController(QObject* parent)
    : QObject(parent)
{
}

void SystemController::reboot()
{
    QProcess::execute("/sbin/reboot", QStringList());
}

void SystemController::resetDefaults()
{
    QProcess::execute("/bin/sh", QStringList() << "/home/root/RobotBrowser/resetDefaults.sh");
}


QString SystemController::systemInfo() const
{
    return QString("robot-browser %1\n"
                   "Qt: %2\n"
                   "Arch: %3\n"
                   "Kernel: %4")
        .arg(APP_VERSION)
        .arg(qVersion())
        .arg(QSysInfo::currentCpuArchitecture())
        .arg(QSysInfo::kernelVersion());
}

bool SystemController::networkAvailable() const
{
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
            && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)
            && !iface.addressEntries().isEmpty()) {
            return true;
        }
    }
    return false;
}
