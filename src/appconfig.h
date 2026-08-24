#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

#include "securitypolicy.h"

// Deployment settings for the browser itself.
//
// Read from /etc/robot-browser/browser.config, a shell-style KEY=VALUE file
// that is already a dpkg conffile, so site edits survive package upgrades. The
// launcher scripts source the same file, so one file describes the terminal.
//
// The package is installed on its own from the CDN and must work without any
// other layer present, so everything it needs to run is here — including the
// URLs. Command-line arguments still win, which is how a provisioning layer
// integrates: the T430/T440 launcher sources its own store and passes the URLs
// it holds, without this package knowing that store exists.
//
// The settings describe what the terminal IS, not what it is pointed at: a
// T430 is wired, a T440 roams on WiFi, a general terminal has both.
class AppConfig {
public:
    // Whether a network type is offered at all.
    enum Availability {
        Auto,       // offer it when NetworkManager reports the device
        On,         // always offer it, even with no device present
        Off         // never offer it, and do not talk to the device
    };

    AppConfig();

    // Later sources win: file over defaults, command line over file.
    void loadFile(const QString& path);
    void applyArgument(const QString& argument);   // --wifi=off, --lan=on, ...

    // Whether the config file named a URL. Not the same question as whether a
    // file exists: the package installs one on every machine, so "has a file"
    // is true even on a desktop that has never been configured. What matters
    // is whether somebody said where this terminal points.
    bool remoteUrlSet() const { return m_remoteUrlSet; }
    bool localUrlSet() const { return m_localUrlSet; }

    Availability wifi() const { return m_wifi; }
    Availability lan() const { return m_lan; }
    // The profile named in the config file, if any. Only the developer build
    // writes this — a terminal is told what it is by its launcher.
    QString profileName() const { return m_profileName; }

    // Security. The mode is what the file asks for; whether the terminal
    // actually locks is SecurityPolicy::locksEnabled(), which lets the server
    // decide when the file says "auto".
    SecurityPolicy::Mode security() const { return m_security; }
    int lockMinutes() const { return m_lockMinutes; }
    void setSecurity(SecurityPolicy::Mode mode) { m_security = mode; }
    void setLockMinutes(int minutes) { m_lockMinutes = minutes; }

    QString remoteUrl() const { return m_remoteUrl; }
    QString localUrl() const { return m_localUrl; }

    void setRemoteUrl(const QString& url) { m_remoteUrl = url; m_remoteUrlSet = true; }
    void setLocalUrl(const QString& url) { m_localUrl = url; m_localUrlSet = true; }

    static QString defaultPath() { return "/etc/robot-browser/browser.config"; }
    static QString describe(Availability value);

private:
    static bool parseAvailability(const QString& text, Availability* out);

    bool m_remoteUrlSet = false;
    bool m_localUrlSet = false;
    Availability m_wifi;
    Availability m_lan;
    QString m_profileName;
    SecurityPolicy::Mode m_security = SecurityPolicy::Auto;
    int m_lockMinutes = SecurityPolicy::kDefaultIdleMinutes;
    QString m_remoteUrl;
    QString m_localUrl;
};

#endif
