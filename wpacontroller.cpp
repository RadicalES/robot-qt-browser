#include "wpacontroller.h"
#include <QDebug>

WpaController::WpaController(QObject* parent)
    : QObject(parent)
    , m_signalLevel(-1)
    , m_connected(false)
{
    // TODO: setup NetworkManager D-Bus polling
    qDebug() << "WPA: stub controller initialized";
}

void WpaController::restartWifi()
{
    // TODO: implement via NetworkManager D-Bus
    qDebug() << "WPA: restartWifi() stub called";
}
