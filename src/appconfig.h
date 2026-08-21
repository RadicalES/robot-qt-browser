#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

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

    // Whether a config file was actually read. A profile's starting URLs are
    // for an install that has no file at all; a terminal with one has already
    // been told what it points at, and must not be overridden by a default.
    bool hasFile() const { return m_hasFile; }

    Availability wifi() const { return m_wifi; }
    Availability lan() const { return m_lan; }
    QString remoteUrl() const { return m_remoteUrl; }
    QString localUrl() const { return m_localUrl; }

    void setRemoteUrl(const QString& url) { m_remoteUrl = url; }
    void setLocalUrl(const QString& url) { m_localUrl = url; }

    static QString defaultPath() { return "/etc/robot-browser/browser.config"; }
    static QString describe(Availability value);

private:
    static bool parseAvailability(const QString& text, Availability* out);

    bool m_hasFile = false;
    Availability m_wifi;
    Availability m_lan;
    QString m_remoteUrl;
    QString m_localUrl;
};

#endif
