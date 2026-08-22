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

    // How much of 1:1 the device viewport is drawn at. Set at startup from the
    // screen it finds itself on, never in the table below.
    //
    // A T440 is 720x1280 portrait, which does not fit on a 1080-tall desktop,
    // so 1:1 was clamped to whatever the screen allowed — quietly changing the
    // viewport the developer was trying to see. Scaling instead keeps the page
    // at the device's own CSS size and draws it smaller: the browser window,
    // the toolbar and the page are all multiplied by this, and the web view
    // gets it as a zoom factor, so the page still lays out as 720x1280.
    qreal scale = 1.0;

    bool toolbar = true;         // the bottom bar
    bool keyboard = true;        // host the virtual keyboard in-process
    bool scadaIndicator = true;  // SCADA state from /run/robot
    bool systemActions = true;   // Reboot and Reset Defaults

    // Whether the Settings dialog is reachable from System Info.
    //
    // Never on a terminal: it is told what it is by its config file and by the
    // layer that provisions it, and a kiosk whose settings an operator can
    // change from the toolbar is not a kiosk. Always on a PC, where pointing
    // the browser at a dev server is the entire workflow.
    bool settings = false;


    // Network controls. A device profile pins these to what the device has; the
    // kiosk profile leaves them to the config file, which is what a deployment
    // uses to say what the terminal IS.
    bool pinNetwork = false;
    AppConfig::Availability wifi = AppConfig::Auto;
    AppConfig::Availability lan = AppConfig::Auto;

    // Where a PC install lands when nobody has said otherwise. A terminal is
    // told its two URLs by its config file or its provisioning layer and these
    // stay empty; a desktop install has neither, and defaulting both to
    // 127.0.0.1 gives a new user two error pages and no clue what the two
    // buttons are for.
    QString remoteUrl;
    QString localUrl;

    // Largest scale at which the device viewport fits the space available,
    // never above 1:1. The margin leaves room for the window's own title bar
    // and a little breathing space, so the window is obviously a window.
    static qreal fitScale(const QSize& want, const QSize& available,
                          qreal margin = 0.92)
    {
        if (!want.isValid() || want.isEmpty() || available.isEmpty())
            return 1.0;
        const qreal byWidth  = qreal(available.width())  * margin / want.width();
        const qreal byHeight = qreal(available.height()) * margin / want.height();
        return qMin(qreal(1.0), qMin(byWidth, byHeight));
    }

    static QStringList names()
    {
        return {"kiosk", "t430", "t431", "t432", "t440", "itpc200", "desktop"};
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

        if (name == "t430" || name == "t431" || name == "t432"
            || name == "t440" || name == "itpc200") {
            // A terminal reproduced on a PC, for someone writing a webapp for
            // it. Windowed at the panel's exact size; device chrome kept so the
            // space it takes is visible; nothing that would act on the
            // developer's own machine.
            //
            //   t430  7" panel,   800x480  landscape — the standard terminal
            //   t431  7" panel,  1024x600  landscape — measured
            //   t432  10" panel, 1280x800  landscape — measured
            //   t440  7" ILI9881,  720x1280 portrait — measured
            //   itpc200 19" HDMI,  1920x1080 landscape — measured
            //
            // The T430 family is one terminal with more than one panel, and
            // the difference is the whole point of a profile: a page laid out
            // for 800x480 has 224 more pixels to fill on a T431. Adding a
            // panel means adding a name here, not stretching an existing one.
            //
            // The T432's panel is natively 800x1280 PORTRAIT and the session
            // rotates it: labwc reports Transform 270 and Xwayland reports
            // 1280x800, which is what Qt sees and therefore what a page gets.
            // The number here is the rotated one for that reason — reading the
            // panel's own mode would give the size of a screen nobody uses.
            //
            // The T440 shares that ILI9881 panel but is 720 wide, not 800, and
            // its session leaves it portrait (Transform: normal). Every one of
            // these four numbers had to be read off a running terminal; not
            // one of them was what the overlay name suggested.
            p.fullscreen = false;
            p.windowSize = (name == "itpc200") ? QSize(1920, 1080)
                         : (name == "t440") ? QSize(720, 1280)
                         : (name == "t432") ? QSize(1280, 800)
                         : (name == "t431") ? QSize(1024, 600)
                                            : QSize(800, 480);
            p.toolbar = true;
            p.keyboard = true;
            p.scadaIndicator = true;
            // The terminal's Reboot reboots the terminal. Here it would reboot
            // the developer's workstation.
            p.systemActions = false;
            // These profiles only ever run on a PC — the terminals themselves
            // run the kiosk profile.
            p.settings = true;
            p.pinNetwork = true;
            // The T430 family is wired and has no radio; a T440 roams on WiFi
            // and its wired port is off. Same two lines as their
            // browser.config.
            p.wifi = (name == "t440") ? AppConfig::On : AppConfig::Off;
            p.lan  = (name == "t440") ? AppConfig::Off : AppConfig::On;
            // The ITPC-200 is an x86 panel PC on Xorg/XFCE rather than a Pi on
            // labwc, and it is wired. Its panel is a full 1920x1080, so on most
            // developer screens this profile is the one that will not fit 1:1
            // and gets scaled.
            *out = p;
            return true;
        }

        if (name == "desktop") {
            // Something useful on first run, and obviously editable after:
            // the product page explains what this is, and Home points at a
            // local web UI, which is what Home means on a terminal.
            p.remoteUrl = "https://radicales.net/";
            p.localUrl = "http://localhost/";
            // A window among other windows on somebody's PC. The desktop owns
            // the network, the keyboard is real, and the machine is not ours
            // to reboot.
            p.fullscreen = false;
            p.toolbar = true;
            p.keyboard = false;
            p.scadaIndicator = false;
            p.systemActions = false;
            p.settings = true;
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
