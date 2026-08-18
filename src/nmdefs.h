#ifndef NMDEFS_H
#define NMDEFS_H

#include <QMap>
#include <QVariant>
#include <QDBusMetaType>

// NetworkManager D-Bus names and the settings types, shared by everything that
// talks to NM: the WiFi controller, the ethernet side, and the IPv4 settings
// used by both.

static const char* NM_SERVICE        = "org.freedesktop.NetworkManager";
static const char* NM_PATH           = "/org/freedesktop/NetworkManager";
static const char* NM_IFACE          = "org.freedesktop.NetworkManager";
static const char* NM_DEVICE         = "org.freedesktop.NetworkManager.Device";
static const char* NM_WIRELESS       = "org.freedesktop.NetworkManager.Device.Wireless";
static const char* NM_WIRED          = "org.freedesktop.NetworkManager.Device.Wired";
static const char* NM_AP             = "org.freedesktop.NetworkManager.AccessPoint";
static const char* NM_SETTINGS       = "org.freedesktop.NetworkManager.Settings";
static const char* NM_SETTINGS_CONN  = "org.freedesktop.NetworkManager.Settings.Connection";
static const char* NM_ACTIVE_CONN    = "org.freedesktop.NetworkManager.Connection.Active";
static const char* NM_IP4CONFIG      = "org.freedesktop.NetworkManager.IP4Config";
static const char* DBUS_PROPS        = "org.freedesktop.DBus.Properties";

// NMDeviceType
static const uint NM_DEVICE_TYPE_ETHERNET = 1;
static const uint NM_DEVICE_TYPE_WIFI     = 2;

// NMDeviceState
static const uint NM_DEVICE_STATE_ACTIVATED = 100;

typedef QMap<QString, QVariant> NMVariantMap;
typedef QMap<QString, NMVariantMap> NMSettingsMap;
Q_DECLARE_METATYPE(NMVariantMap)
Q_DECLARE_METATYPE(NMSettingsMap)

#endif
