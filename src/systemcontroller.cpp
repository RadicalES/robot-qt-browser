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
    return QString("RobotBrowser Version 2.0\n"
                   "Qt: %1\n"
                   "Arch: %2\n"
                   "Kernel: %3")
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
