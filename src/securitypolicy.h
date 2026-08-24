#ifndef SECURITYPOLICY_H
#define SECURITYPOLICY_H

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QString>

// Whether this terminal locks, and who may unlock it.
//
// A terminal in a packhouse is a shared machine: it runs unattended between
// transactions, and anyone walking past can use whatever the last operator
// left open. So it behaves like a desktop — it asks who you are before it will
// show the page, and it locks itself again when nobody has touched it.
//
// **Where the decision comes from.** The SCADA server already carries it: a
// terminal's configuration has a `security` field, OPEN or SECURE, which
// robot-scada-client writes into /run/robot/setup.json when the device is
// onboarded. That is the authority — a site turns security on for a terminal
// on the server, not by editing a file on the terminal. A device that has
// never been onboarded has no setup.json, and then the config file decides.
//
// **Who may unlock it, for now.** A local account, hashed in the config file
// the deployment controls. The server grows an authentication endpoint later
// (it already has /api/v1/auth/login/ issuing JWTs, and robot-scada-client
// will publish its URL beside serverURL); this class is where that goes, and
// the local account stays as the fallback for a terminal that is not onboarded
// or cannot reach its server — which is exactly when somebody needs to get
// into it.
class SecurityPolicy {
public:
    enum Mode {
        Auto,   // follow the server's setting, unlocked if it has none
        On,     // always lock, whatever the server says
        Off,    // never lock — the browser as it was before this existed
    };

    // Reads /run/robot/setup.json, the file robot-scada-client publishes.
    //
    // Absent means one of three things and they are not distinguishable here:
    // the client is not installed, it is installed and has never reached a
    // server, or the device is not onboarded. All three mean the same for us:
    // there is no server instruction, so the config file decides.
    static bool serverRequiresLock(bool* known = nullptr)
    {
        if (known)
            *known = false;

        QFile file("/run/robot/setup.json");
        if (!file.open(QIODevice::ReadOnly))
            return false;

        const QJsonObject setup =
                QJsonDocument::fromJson(file.readAll()).object();
        if (!setup.contains("security"))
            return false;

        if (known)
            *known = true;

        // The server's own vocabulary. The model offers OPEN and SECURE; the
        // packet documentation says "OPEN/REQUIRED", so both spellings of "on"
        // are accepted rather than one of them silently meaning "off".
        const QString value = setup.value("security").toString().trimmed().toUpper();
        return value == "SECURE" || value == "REQUIRED";
    }

    // The mode a terminal actually runs in, config over server.
    static bool locksEnabled(Mode configured)
    {
        switch (configured) {
        case On:  return true;
        case Off: return false;
        case Auto:
        default:  return serverRequiresLock();
        }
    }

    static bool parseMode(const QString& text, Mode* out)
    {
        const QString value = text.trimmed().toLower();
        if (value == "auto")                      { *out = Auto; return true; }
        if (value == "on" || value == "true")     { *out = On;   return true; }
        if (value == "off" || value == "false")   { *out = Off;  return true; }
        return false;
    }

    static QString describe(Mode mode)
    {
        switch (mode) {
        case On:  return "on";
        case Off: return "off";
        case Auto:
        default:  return "auto";
        }
    }

    // --- the local account ---------------------------------------------------
    //
    // Stored as an iterated salted hash, never as the password. A terminal's
    // config file is readable by anyone who can read the filesystem, and the
    // same password is likely to be used across a site's terminals — one
    // recovered password would be every terminal's password.
    //
    // Format: pbkdf2-sha256$<iterations>$<salt-hex>$<hash-hex>
    //
    // A plain password is also accepted, because a site that has just imaged a
    // terminal by hand should not be locked out by a hashing format they have
    // to generate; it is upgraded to a hash the first time it is used.
    static QString hash(const QString& password, const QByteArray& salt,
                        int iterations = kIterations)
    {
        QByteArray value = salt + password.toUtf8();
        for (int i = 0; i < iterations; ++i)
            value = QCryptographicHash::hash(value, QCryptographicHash::Sha256);

        return QString("pbkdf2-sha256$%1$%2$%3")
                .arg(iterations)
                .arg(QString::fromLatin1(salt.toHex()))
                .arg(QString::fromLatin1(value.toHex()));
    }

    static QString hash(const QString& password)
    {
        return hash(password, randomSalt());
    }

    static bool isHashed(const QString& stored)
    {
        return stored.startsWith("pbkdf2-sha256$");
    }

    // Constant-time-ish comparison: QByteArray::operator== stops at the first
    // difference, which leaks how much of a hash matched. Not a serious attack
    // against a terminal in a packhouse, but the cost of doing it properly is
    // one loop.
    static bool verify(const QString& password, const QString& stored)
    {
        if (stored.isEmpty())
            return false;

        if (!isHashed(stored))
            return equal(password.toUtf8(), stored.toUtf8());

        const QStringList parts = stored.split('$');
        if (parts.size() != 4)
            return false;

        const int iterations = parts.at(1).toInt();
        const QByteArray salt = QByteArray::fromHex(parts.at(2).toLatin1());
        if (iterations <= 0 || salt.isEmpty())
            return false;

        return equal(hash(password, salt, iterations).toUtf8(), stored.toUtf8());
    }

    static QByteArray randomSalt()
    {
        QByteArray salt(16, 0);
        for (int i = 0; i < salt.size(); ++i)
            salt[i] = char(QRandomGenerator::system()->bounded(256));
        return salt;
    }

    static constexpr int kIterations = 100000;

    // What a terminal locks after, with nobody touching it. Minutes, because
    // that is the unit the person setting it thinks in.
    static constexpr int kDefaultIdleMinutes = 5;

private:
    static bool equal(const QByteArray& a, const QByteArray& b)
    {
        if (a.size() != b.size())
            return false;

        unsigned char difference = 0;
        for (int i = 0; i < a.size(); ++i)
            difference |= static_cast<unsigned char>(a.at(i) ^ b.at(i));
        return difference == 0;
    }
};

#endif
