#ifndef RUNPROFILE_H
#define RUNPROFILE_H

#include <QSize>
#include <QString>
#include <QStringList>

#include "appconfig.h"

// How this instance is meant to run.
//
// The browser started as one thing: the whole of a terminal. Fullscreen on the
// panel's geometry, a toolbar it owns, an on-screen keyboard because the
// session has no other, a SCADA indicator reading /run/robot, and a Reboot
// button that reboots the terminal. All of that is right on a T430 or a T440
// and wrong everywhere else, and "everywhere else" now includes a desktop PC
// and a webapp developer's laptop.
//
// A profile is the whole set of those answers under one name, rather than a
// handful of flags the caller has to get consistently right. `--profile=t440`
// reproduces a T440: its panel geometry, its network controls, its chrome —
// so a page that overflows on the device overflows here too.
//
// Two rules the table below follows:
//
//   Nothing that cannot work is shown. A Reboot button on a developer's laptop
//   is either a lie or a disaster, and a WiFi dialog on a terminal with no
//   radio is a dead end.
//
//   Everything that shapes the viewport is kept. The toolbar eats 76px on the
//   device, so a device profile keeps the toolbar even though the developer
//   cannot use most of it — the point is what the page has left, not what the
//   buttons do.
struct RunProfile {
    QString name;

    // Geometry. Kiosk takes the panel; everything else is a window, because a
    // fullscreen window with no exit traps whoever opened it.
    bool fullscreen = true;
    QSize windowSize;            // used when !fullscreen; empty = let it be

    bool toolbar = true;         // the bottom bar
    bool keyboard = true;        // host the virtual keyboard in-process
    bool scadaIndicator = true;  // SCADA state from /run/robot
    bool systemActions = true;   // Reboot and Reset Defaults

    // Network controls. A device profile pins these to what the device has; the
    // kiosk profile leaves them to the config file, which is what a deployment
    // uses to say what the terminal IS.
    bool pinNetwork = false;
    AppConfig::Availability wifi = AppConfig::Auto;
    AppConfig::Availability lan = AppConfig::Auto;

    static QStringList names()
    {
        return {"kiosk", "t430", "t440", "desktop"};
    }

    // Unknown names are the caller's mistake and are reported, not guessed at:
    // a typo in a profile name would otherwise silently give a developer a
    // window of the wrong size and a wrong impression of their own page.
    static bool lookup(const QString& name, RunProfile* out)
    {
        RunProfile p;
        p.name = name;

        if (name == "kiosk") {
            // The terminal itself. Everything on, geometry from the panel.
            *out = p;
            return true;
        }

        if (name == "t430" || name == "t440") {
            // A terminal reproduced on a PC, for someone writing a webapp for
            // it. Windowed at the panel's exact size; device chrome kept so the
            // space it takes is visible; nothing that would act on the
            // developer's own machine.
            //
            // GEOMETRY IS PROVISIONAL. These are read off the panel overlays in
            // the rootfs repos, not measured on hardware:
            //   t430  vc4-kms-dsi-7inch         official 7" DSI, 800x480
            //   t440  vc4-kms-dsi-ili9881-7inch ILI9881 7", 800x1280 portrait
            // Confirm with QScreen::geometry() on each terminal — a profile
            // whose whole purpose is an exact viewport must not ship on a
            // guess. See issue #8.
            p.fullscreen = false;
            p.windowSize = (name == "t430") ? QSize(800, 480) : QSize(800, 1280);
            p.toolbar = true;
            p.keyboard = true;
            p.scadaIndicator = true;
            // The terminal's Reboot reboots the terminal. Here it would reboot
            // the developer's workstation.
            p.systemActions = false;
            p.pinNetwork = true;
            // A T430 is wired and has no radio; a T440 roams on WiFi and its
            // wired port is off. Same two lines as their browser.config.
            p.wifi = (name == "t440") ? AppConfig::On : AppConfig::Off;
            p.lan  = (name == "t430") ? AppConfig::On : AppConfig::Off;
            *out = p;
            return true;
        }

        if (name == "desktop") {
            // A window among other windows on somebody's PC. The desktop owns
            // the network, the keyboard is real, and the machine is not ours
            // to reboot.
            p.fullscreen = false;
            p.toolbar = true;
            p.keyboard = false;
            p.scadaIndicator = false;
            p.systemActions = false;
            p.pinNetwork = true;
            p.wifi = AppConfig::Off;
            p.lan = AppConfig::Off;
            *out = p;
            return true;
        }

        return false;
    }
};

#endif
