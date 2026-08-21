#include "appconfig.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

AppConfig::AppConfig()
    : m_wifi(Auto)
    , m_lan(Auto)
    , m_remoteUrl("http://127.0.0.1")
    , m_localUrl("http://127.0.0.1")
{
    // Auto by default: a terminal offers what it physically has. A deployment
    // only needs to say otherwise when it wants a link type hidden despite the
    // hardware being present, or shown despite it being absent.
}

bool AppConfig::parseAvailability(const QString& text, Availability* out)
{
    const QString value = text.trimmed().toLower();
    if (value == "auto")                              { *out = Auto; return true; }
    if (value == "on"  || value == "yes" || value == "1")  { *out = On;  return true; }
    if (value == "off" || value == "no"  || value == "0")  { *out = Off; return true; }
    return false;
}

QString AppConfig::describe(Availability value)
{
    switch (value) {
    case On:  return "on";
    case Off: return "off";
    default:  return "auto";
    }
}

void AppConfig::loadFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Absent is normal: the defaults describe a general terminal, and a
        // desktop install has no such file at all.
        qDebug() << "AppConfig: no config at" << path << "— using defaults";
        return;
    }
    m_hasFile = true;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        const int eq = line.indexOf('=');
        if (eq < 0)
            continue;

        const QString key = line.left(eq).trimmed().toUpper();
        QString value = line.mid(eq + 1).trimmed();
        // The file is shell-sourced by the launchers too, so values may be
        // quoted.
        if (value.length() >= 2
            && ((value.startsWith('"') && value.endsWith('"'))
                || (value.startsWith('\'') && value.endsWith('\'')))) {
            value = value.mid(1, value.length() - 2);
        }

        // WB_* names match what the launcher scripts already source from this
        // same file, so one file serves both.
        if (key == "WB_REMOTE_URL") {
            if (!value.isEmpty())
                m_remoteUrl = value;
            continue;
        }
        if (key == "WB_LOCAL_URL") {
            if (!value.isEmpty())
                m_localUrl = value;
            continue;
        }

        Availability parsed;
        if (key == "NETWORK_WIFI") {
            if (parseAvailability(value, &parsed))
                m_wifi = parsed;
            else
                qWarning() << "AppConfig: NETWORK_WIFI value not understood:" << value;
        } else if (key == "NETWORK_LAN") {
            if (parseAvailability(value, &parsed))
                m_lan = parsed;
            else
                qWarning() << "AppConfig: NETWORK_LAN value not understood:" << value;
        }
    }

    qInfo().noquote() << QString("AppConfig: %1 — wifi=%2 lan=%3 remote=%4 local=%5")
                             .arg(path, describe(m_wifi), describe(m_lan),
                                  m_remoteUrl, m_localUrl);
}

void AppConfig::applyArgument(const QString& argument)
{
    const QStringList parts = argument.split('=');
    if (parts.size() != 2)
        return;

    // Only the settings this class owns. Other flags — --profile, --config,
    // --windowed — are handled where they belong, and complaining about them
    // here produced a warning about a value that is perfectly valid for the
    // argument it actually belongs to.
    const QString name = parts.at(0);
    if (name != "--wifi" && name != "--lan")
        return;

    Availability parsed;
    if (!parseAvailability(parts.at(1), &parsed)) {
        qWarning() << "AppConfig: value not understood:" << argument;
        return;
    }

    if (name == "--wifi")
        m_wifi = parsed;
    else
        m_lan = parsed;
}
