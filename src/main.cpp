#include <QApplication>
#include <QIcon>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QInputMethod>
#include <QLabel>
#include <QDialog>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTimer>
#include <QAction>
#include <QProcess>
#include <QScreen>
#include <QSizeF>
#include <QDir>
#include <QDebug>


#include "webpagecontroller.h"
#include "networkcontroller.h"
#include "systemcontroller.h"
#include "websockserver.h"
#include "unixsignalnotifier.h"
#include "digitalclock.h"
#include "wifidialog.h"
#include "landialog.h"
#include "infodialog.h"
#include "virtualkeyboardpanel.h"
#include "pagefocusguard.h"
#include "scadaindicator.h"
#include "runprofile.h"
#include "settingsdialog.h"
#include "robothead.h"
#include "dialogstyle.h"
#include "kioskstyle.h"
#include "appconfig.h"

// Both bottom-docked keyboards are four rows, and a touch key wants about
// 11mm — the size of a fingertip, not a fraction of whatever screen it is on.
static constexpr int kRows = 4;
static constexpr qreal kKeyMillimetres = 11.0;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    // Raise the keyboard on the FIRST tap of an input, not the second.
    app.setStyle(new KioskStyle);
    app.setOrganizationName("Radical Electronic Systems");
    app.setApplicationName("RobotBrowser");
    app.setApplicationVersion(APP_VERSION);

    // The product mark, orange — the same head the toolbar shows in its
    // neutral state, and the icon the package installs for the menu entry.
    //
    // Both lines matter and neither replaces the other. setWindowIcon dresses
    // the window itself, which is what X11 taskbars read. Wayland ignores it
    // entirely and instead matches the surface's app-id to a .desktop file, so
    // without setDesktopFileName a GNOME dock finds no entry, falls back to
    // whatever it cached for this binary, and keeps showing an icon shipped by
    // a version that is no longer installed.
    app.setWindowIcon(QIcon(":/images/robot-head.svg"));
    app.setDesktopFileName("robot-browser");

    // Engine settings and persistent storage are configured per-profile in
    // WebPageController — QtWebEngine has no equivalent of QWebSettings globals.

    // Settings: built-in defaults, then the package's own config file, then the
    // command line. The package is installed standalone from the CDN, so it
    // runs from the file alone; a provisioning layer integrates by passing what
    // it holds as arguments, without this needing to know that layer exists.
    //
    //   robot-browser [--profile=kiosk|t430|t440|desktop] [--config=PATH]
    //                 [--wifi=auto|on|off] [--lan=...]
    //                 [--windowed[=WxH]] [--no-toolbar] [remote_url] [local_url]
    AppConfig config;
    QStringList positional;
    // A profile says how this instance is meant to run — see runprofile.h. The
    // default is the terminal: fullscreen, toolbar, keyboard, the lot.
    //
    // The individual flags still work and still win, because they are how the
    // desktop launchers open the browser as a single-purpose tool (the WiFi
    // setup page opens it windowed with no toolbar), and because a developer
    // will want a device profile at some size other than 1:1 sooner or later.
    //
    //   --profile=NAME     kiosk (default), t430, t440, desktop, or "user"
    //                      for the one saved in Settings
    //   --scale=N|fit|1    how large to draw a device profile (see below)
    //   --settings=on|off  offer "Settings…" in System Info
    //   --keyboard=auto|full|standard|narrow|off   which keyboard to render
    //   --windowed[=WxH]   a normal window the compositor can decorate and close
    //   --no-toolbar       drop the bottom bar; the page is the whole point
    RunProfile profile;
    RunProfile::lookup("kiosk", &profile);
    bool windowedFlag = false;
    bool noToolbarFlag = false;
    QSize windowSizeFlag;
    double scaleFlag = -1.0;        // < 0 means "not given"
    int settingsFlag = -1;          // < 0 means "not given"
    QString keyboardFlag;
    bool profileFlagGiven = false;
    QString configPath = AppConfig::defaultPath();
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i);
        if (arg.startsWith("--config="))
            configPath = arg.mid(strlen("--config="));
        else if (arg.startsWith("--profile=")) {
            const QString name = arg.section('=', 1);
            // "user" is not a profile — it means "whatever the Settings dialog
            // last saved, and desktop until it has saved anything". The menu
            // entry passes it, so a device chosen in Settings survives a
            // restart; every other name is a deliberate choice that outranks
            // the saved one, so --profile=desktop still means desktop.
            if (name == "user") {
                RunProfile::lookup("desktop", &profile);
                continue;
            }
            profileFlagGiven = true;
            if (!RunProfile::lookup(name, &profile)) {
                // Not guessed at: a typo would otherwise hand a developer a
                // window of the wrong size and a wrong impression of their page.
                fprintf(stderr, "robot-browser: unknown profile '%s' (known: %s, user)\n",
                        qPrintable(name),
                        qPrintable(RunProfile::names().join(", ")));
                return 2;
            }
        }
        else if (arg == "--windowed" || arg.startsWith("--windowed=")) {
            windowedFlag = true;
            if (arg.contains('=')) {
                const QStringList wh = arg.section('=', 1).split('x');
                if (wh.size() == 2)
                    windowSizeFlag = QSize(wh.at(0).toInt(), wh.at(1).toInt());
            }
        }
        else if (arg == "--no-toolbar")
            noToolbarFlag = true;
        else if (arg.startsWith("--keyboard=")) {
            const QString value = arg.section('=', 1);
            if (value == "off") {
                keyboardFlag = value;
            } else if (value == "auto" || value == "full"
                       || value == "standard" || value == "narrow") {
                keyboardFlag = value;
            } else {
                fprintf(stderr, "robot-browser: --keyboard wants auto, full, "
                                "standard, narrow or off\n");
                return 2;
            }
        }
        else if (arg.startsWith("--settings=")) {
            const QString value = arg.section('=', 1);
            if (value != "on" && value != "off") {
                fprintf(stderr, "robot-browser: --settings wants on or off\n");
                return 2;
            }
            settingsFlag = (value == "on") ? 1 : 0;
        }
        else if (arg.startsWith("--scale=")) {
            const QString value = arg.section('=', 1);
            if (value == "fit")
                scaleFlag = 0.0;            // recomputed from the screen below
            else if (value == "1" || value == "1.0")
                scaleFlag = 1.0;            // 1:1, even if it does not fit
            else {
                bool ok = false;
                const double parsed = value.toDouble(&ok);
                if (!ok || parsed <= 0.0 || parsed > 1.0) {
                    fprintf(stderr, "robot-browser: --scale wants a fraction "
                                    "0 < N <= 1, or 'fit'\n");
                    return 2;
                }
                scaleFlag = parsed;
            }
        }
        else if (!arg.startsWith("--"))
            positional << arg;
    }

    if (windowedFlag) {
        profile.fullscreen = false;
        if (windowSizeFlag.isValid())
            profile.windowSize = windowSizeFlag;
    }
    if (noToolbarFlag)
        profile.toolbar = false;
    if (settingsFlag >= 0)
        profile.settings = (settingsFlag == 1);
    if (!keyboardFlag.isEmpty()) {
        if (keyboardFlag == "off") {
            profile.keyboard = false;
        } else {
            // Naming a layout turns the keyboard ON as well as choosing it.
            // The desktop profile has none by default — a PC has a real
            // keyboard — but a touch-screen PC is still a PC, and
            // "--profile=desktop --keyboard=full" should mean what it says.
            profile.keyboard = true;
            profile.keyboardLayout = keyboardFlag;
        }
    }


    config.loadFile(configPath);
    // Then this user's own settings, which the Settings dialog writes. Only
    // where that dialog exists: a terminal must take its configuration from
    // the file its deployment controls, and nothing else.
    if (profile.settings)
        config.loadFile(SettingsDialog::savePath());

    // A profile saved by the Settings dialog, applied when the command line did
    // not name one. The flag still wins: a launcher that says --profile=t440
    // means it, whatever a developer last picked in the dialog.
    //
    // This has to happen BEFORE the scale is worked out below. It used to come
    // after, so a saved t440 arrived too late to be fitted to the screen and
    // opened at a size the desktop then refused - the device chosen in
    // Settings behaved differently from the same device named on the command
    // line, which is the one thing a saved setting must never do.
    if (!profileFlagGiven && !config.profileName().isEmpty()) {  // --profile=user
        RunProfile saved;
        if (RunProfile::lookup(config.profileName(), &saved))
            profile = saved;
    }

    // How large to draw a device profile.
    //
    // A T440 is 800x1280 portrait and does not fit on a 1080-tall desktop. It
    // used to be clamped to whatever the screen allowed, which silently
    // changed the viewport the developer was trying to look at. Scaling keeps
    // the page at the device's own size — the window, the chrome and the page
    // are all multiplied by this, and the web view gets it as a zoom factor —
    // so the layout is the device's and only the physical size differs.
    if (!profile.fullscreen && profile.windowSize.isValid()) {
        const QSize available = app.primaryScreen()->availableGeometry().size();
        if (scaleFlag > 0.0)
            profile.scale = scaleFlag;
        else
            profile.scale = RunProfile::fitScale(profile.windowSize, available);

        if (profile.scale < 1.0) {
            fprintf(stderr, "robot-browser: profile '%s' is %dx%d, drawn at %d%% "
                            "to fit this screen (%dx%d). The page still lays out "
                            "as %dx%d.\n",
                    qPrintable(profile.name),
                    profile.windowSize.width(), profile.windowSize.height(),
                    int(profile.scale * 100 + 0.5),
                    available.width(), available.height(),
                    profile.windowSize.width(), profile.windowSize.height());
        }
    }

    // The dialogs read their own sizes from here, and they are built further
    // down, so this has to be set before any of them exists.
    DialogStyle::setScale(profile.scale);

    // Toolbar metrics follow the scale, so the page keeps the same share of
    // the window it has on the device. Chrome that stayed 76px tall over a
    // scaled page would take a bigger bite than it does on the terminal, and
    // the preview would be wrong in exactly the way it exists to prevent.
    const auto px = [&profile](int deviceValue) {
        return qMax(1, int(qRound(deviceValue * profile.scale)));
    };
    if (profile.pinNetwork) {
        // A device profile reproduces a viewport, not a terminal: it shows no
        // network indicators at all. Pinning them rather than leaving them to
        // this machine's config file is what makes that true wherever it runs
        // — otherwise a developer's own browser.config could put a WiFi icon
        // in a preview of a wired terminal.
        config.applyArgument("--wifi=" + AppConfig::describe(profile.wifi));
        config.applyArgument("--lan=" + AppConfig::describe(profile.lan));
    }
    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i).startsWith("--"))
            config.applyArgument(args.at(i));
    }
    // A profile's URLs sit under the config file and the command line: they are
    // a starting point for an install that has neither, not a policy.
    if (!profile.remoteUrl.isEmpty() && !config.remoteUrlSet())
        config.setRemoteUrl(profile.remoteUrl);
    if (!profile.localUrl.isEmpty() && !config.localUrlSet())
        config.setLocalUrl(profile.localUrl);
    if (positional.size() > 0)
        config.setRemoteUrl(positional.at(0));
    if (positional.size() > 1)
        config.setLocalUrl(positional.at(1));

    const QUrl remoteUrl(config.remoteUrl());
    const QUrl localUrl(config.localUrl());

    // Unix signal handling for systemd
    UnixSignalNotifier::instance()->installSignalHandler(SIGINT);
    UnixSignalNotifier::instance()->installSignalHandler(SIGTERM);
    QObject::connect(UnixSignalNotifier::instance(), SIGNAL(unixSignal(int)),
                     &app, SLOT(quit()));

    // Debug WebSocket server
    WebsockServer debugSvr(7070, false, &app);

    // C++ backend controllers
    NetworkController networkController(config.wifi() != AppConfig::Off,
                                        config.lan() != AppConfig::Off);
    SystemController systemController;
    WebPageController webPageController;
    webPageController.init(localUrl, remoteUrl, &debugSvr);

    // Main window — web view fills the window, with the virtual keyboard
    // sharing the central area so it pushes content up rather than covering it.
    QMainWindow window;

    // Where the keyboard goes, and how tall the toolbar is, both follow the
    // shape of the panel.
    //
    // The keyboard's height is a fixed share of its width, so on a short
    // landscape panel a full-width keyboard leaves almost nothing to type
    // into: a T431 is 1024x600 and the keyboard wants 476 of those 600 pixels.
    // Docking it down the side instead costs width, which a landscape panel
    // has, rather than height, which it does not.
    const QSize panel = profile.windowSize.isValid()
        ? profile.windowSize
        : app.primaryScreen()->geometry().size();
    // "auto" asks the panel; anything else was asked for explicitly and is
    // taken at its word, even where it is a poor fit — a deployment that says
    // --keyboard=full on a small screen has its reasons.
    const QString wanted = profile.keyboardLayout == "auto"
        ? (panel.width() > panel.height() && panel.height() < 760
               ? QStringLiteral("narrow")
               : panel.width() >= 1200 ? QStringLiteral("full")
                                       : QStringLiteral("standard"))
        : profile.keyboardLayout;

    const bool sideKeyboard = (wanted == "narrow");

    // Three keyboards, chosen by pointing QtVirtualKeyboard at a different
    // layout directory:
    //
    //   narrow    four across, down the side. A short landscape panel has no
    //             room under the page: a third of 1024px is 341px, and ten
    //             keys across that is 34px each, unhittable however the text
    //             is scaled. The arrangement has to change, not the size.
    //   full      twelve across with numbers and symbols on the same page, for
    //             a panel wide enough that switching pages is a limitation
    //             with no cause.
    //   standard  the package's own ten-across, for everything else.
    //
    // Decided here rather than in the launcher because the browser knows the
    // panel; a launcher would have to be told, per terminal, and would be
    // wrong the moment a unit shipped with a different screen. --keyboard=
    // overrides it for a deployment that knows better.
    // A wide panel has room for every key at once: numbers, letters and the
    // everyday symbols, with no page to switch. A T432 is 1280x800 and an
    // ITPC-200 is 1920x1080; making an operator press SYM to type a digit on
    // one of those is a limitation with no cause.
    const bool fullKeyboard = (wanted == "full");

    if (profile.keyboard) {
        const QString path = sideKeyboard
            ? QStringLiteral("/usr/share/robot-browser/layouts-narrow")
            : fullKeyboard ? QStringLiteral("/usr/share/robot-browser/layouts-full")
                           : QString();   // standard: the package's own layouts
        if (!path.isEmpty() && QDir(path).exists())
            qputenv("QT_VIRTUALKEYBOARD_LAYOUT_PATH", path.toUtf8());
        if (qEnvironmentVariableIsSet("ROBOT_BROWSER_DEBUG_GEOMETRY")) {
            fprintf(stderr, "keyboard: %s (panel %dx%d)\n",
                    qPrintable(wanted), panel.width(), panel.height());
        }
    }

    QWidget* central = new QWidget;
    // Black, the same ground the keyboard draws on, so the space around a
    // keyboard that does not span the full width — and any gap the page does
    // not fill — reads as part of the terminal rather than as a light band
    // nobody chose.
    central->setAutoFillBackground(true);
    central->setStyleSheet("background: black;");

    // The page column: banner, page, load bar — and the keyboard too, unless
    // it is going down the side.
    QWidget* pageColumn = new QWidget;
    QVBoxLayout* centralLayout = new QVBoxLayout(pageColumn);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    QHBoxLayout* centralRow = new QHBoxLayout(central);
    centralRow->setContentsMargins(0, 0, 0, 0);
    centralRow->setSpacing(0);
    centralRow->addWidget(pageColumn, 1);
    // Offline notice: an inline strip rather than a modal. A packhouse dead
    // zone is routine, and a dialog would block the operator and steal focus
    // from the page for something that fixes itself when they move.
    QLabel* offlineBanner = new QLabel(
        "No network — the page could not be loaded. It will work again once "
        "the terminal is back in range.");
    offlineBanner->setWordWrap(true);
    offlineBanner->setStyleSheet(
        QString("background: #ffa726; color: #4d4d4d; font-size: %1px; "
                "padding: %2px;").arg(px(20)).arg(px(14)));
    offlineBanner->hide();
    centralLayout->addWidget(offlineBanner);

    centralLayout->addWidget(webPageController.webView(), 1);

    // Load progress, sitting on the bottom edge of the page. Without it,
    // tapping Remote on a slow link looks like nothing happened and the
    // operator taps again.
    //
    // Always present, never hidden: showing and hiding it resized the page
    // under the operator's finger. Left full when idle, it reads as a divider
    // between the page and the toolbar.
    QProgressBar* loadBar = new QProgressBar;
    loadBar->setRange(0, 100);
    loadBar->setValue(100);
    loadBar->setTextVisible(false);
    loadBar->setFixedHeight(6);
    loadBar->setStyleSheet(
        "QProgressBar { background: #2b2b2b; border: none; }"
        "QProgressBar::chunk { background: #ff4500; }");
    centralLayout->addWidget(loadBar);

    // The on-screen keyboard exists because a terminal has no other one. On a
    // desktop there is a real keyboard, and loading this costs a QQuickWidget,
    // the QtVirtualKeyboard QML module and its dependencies — which a desktop
    // install has no reason to have present at all.
    VirtualKeyboardPanel* keyboard = profile.keyboard ? new VirtualKeyboardPanel : nullptr;
    if (keyboard) {
        if (sideKeyboard)
            centralRow->addWidget(keyboard, 0, Qt::AlignBottom);
        else
            centralLayout->addWidget(keyboard, 0, Qt::AlignHCenter);
    }

    window.setCentralWidget(central);

    // Bottom toolbar. Sized for finger use on a touch-only terminal: 44px
    // with 34px icons was too small a target to hit reliably.
    QToolBar* toolbar = new QToolBar(&window);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    // A tenth of the panel's height, floored at a size a finger can still hit
    // and capped by the shape of the screen.
    //
    // On a portrait panel there is height to spare and 76px is right. On a
    // landscape one height is the scarce dimension — it is shared with the
    // page and the keyboard — so the bar stops at 60px there: 60 on a T431 and
    // a T432, 48 on a standard 800x480 T430, and 76 unchanged on a T440 or a
    // desktop.
    const int barCap = (panel.width() > panel.height()) ? px(60) : px(76);
    const int barHeight = qBound(px(44), panel.height() / 10, barCap);
    const int barIcon = qBound(px(28), barHeight * 5 / 8, px(48));
    toolbar->setIconSize(QSize(barIcon, barIcon));
    toolbar->setFixedHeight(barHeight);
    // Tight spacing: with WiFi, LAN and the SCADA head all present the row is
    // nine buttons plus the clock, which at a comfortable 10px gap and 60px
    // buttons overflows a 720px panel. The 48px icons still give a finger-sized
    // target on their own, so the padding around them is what gives way.
    toolbar->setStyleSheet(QString(
        "QToolBar { background: #4d4d4d; spacing: %1px; padding: %1px; border: none; }"
        "QToolButton { border: none; padding: %2px; min-width: %3px; }"
        "QToolButton:pressed { background: #808080; border-radius: 6px; }")
        .arg(px(2)).arg(px(3)).arg(px(54)));
    window.addToolBar(Qt::BottomToolBarArea, toolbar);

    // Navigation buttons
    QAction* homeAction = toolbar->addAction(QIcon(":/images/home.png"), "");
    QAction* remoteAction = toolbar->addAction(QIcon(":/images/store.svg"), "");
    QAction* backAction = toolbar->addAction(QIcon(":/images/back.png"), "");

    // Spacer
    QWidget* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // WiFi icon button
    const bool showWifi = (config.wifi() == AppConfig::On)
        || (config.wifi() == AppConfig::Auto);
    const bool showLan = (config.lan() == AppConfig::On)
        || (config.lan() == AppConfig::Auto);

    QToolButton* wifiButton = new QToolButton;
    wifiButton->setIconSize(QSize(barIcon, barIcon));
    wifiButton->setAutoRaise(true);
    wifiButton->setIcon(QIcon(":/images/wifi-off.png"));
    // "auto" means offer it when the hardware is there — the same rule the LAN
    // button follows. It was shown unconditionally, so a wired terminal with no
    // radio, or one whose radio the deployment does not use, still had a WiFi
    // button that opened a dialog with nothing in it. An ITPC-200 is wired.
    wifiButton->setVisible(config.wifi() == AppConfig::On
                           || (showWifi && !networkController.wifiDevicePath().isEmpty()));
    toolbar->addWidget(wifiButton);

    // LAN icon button, beside WiFi: the T430 terminals run wired, so the
    // operator needs the same at-a-glance state and the same way in to
    // settings for either link.
    QToolButton* lanButton = new QToolButton;
    lanButton->setIconSize(QSize(barIcon, barIcon));
    lanButton->setAutoRaise(true);
    lanButton->setIcon(QIcon(":/images/lan-down.png"));
    lanButton->setVisible(showLan && networkController.lanAvailable());
    toolbar->addWidget(lanButton);

    // SCADA server status, mirroring the desktop tray indicator: grey when no
    // server or service is detected, orange when the terminal is not
    // provisioned, green when it is talking to a server.
    // Device chrome: kept in a device profile even off-device, because the
    // space it takes is part of what the page has to live with. Dropped on a
    // plain desktop, where there is no /run/robot and nothing it could report.
    ScadaIndicator* scadaIndicator = profile.scadaIndicator ? new ScadaIndicator : nullptr;
    if (scadaIndicator) {
        scadaIndicator->setIconPixels(barIcon);
        toolbar->addWidget(scadaIndicator);
    }

    // Manual keyboard toggle. The keyboard normally follows the focused field,
    // but an operator needs a way to dismiss it while reading, and a way to
    // raise it if it fails to appear.
    QToolButton* keyboardButton = new QToolButton;
    keyboardButton->setIconSize(QSize(barIcon, barIcon));
    keyboardButton->setAutoRaise(true);
    keyboardButton->setIcon(QIcon(":/images/keyboard.png"));
    keyboardButton->setVisible(profile.keyboard);
    toolbar->addWidget(keyboardButton);

    // Clock
    DigitalClock* clock = new DigitalClock;
    clock->setFontPixels(px(24));
    toolbar->addWidget(clock);

    // Info button
    QAction* infoAction = toolbar->addAction(QIcon(":/images/info.png"), "");

    // Dialogs
    WifiDialog* wifiDialog = new WifiDialog(&networkController, &window);
    LanDialog* lanDialog = new LanDialog(&networkController, &window);
    // Reboot and Reset Defaults act on the machine this runs on. On a terminal
    // that is the terminal; on a developer's PC it is their workstation.
    InfoDialog* infoDialog = new InfoDialog(&systemController, &window);
    infoDialog->setSystemActionsVisible(profile.systemActions);
    if (profile.settings) {
        infoDialog->addSettingsButton([&window, &config, &webPageController,
                                       &profile, &app]() {
            SettingsDialog dialog(config.remoteUrl(), config.localUrl(),
                                  profile.name, &window);
            DialogStyle::closeKeyboard();
            if (dialog.exec() != QDialog::Accepted)
                return;

            // Changing the device means starting again: geometry, chrome and
            // keyboard layout are all settled at startup, and pretending
            // otherwise would show a half-changed terminal.
            if (dialog.profileName() != profile.name) {
                // --profile=user, not the name just chosen: the dialog has
                // already saved it, and restarting through the saved value is
                // the same path the next launch from the menu takes. Passing
                // the name would work now and diverge later, which is how a
                // setting comes to look like it did not persist.
                QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                        {"--profile=user"});
                app.quit();
                return;
            }

            config.setRemoteUrl(dialog.remoteUrl());
            config.setLocalUrl(dialog.localUrl());
            webPageController.setUrls(QUrl(config.localUrl()),
                                      QUrl(config.remoteUrl()));
            // Straight to the new remote page: the reason to change these is
            // to look at what they point at.
            webPageController.loadRemote();
            window.raise();
            window.activateWindow();
        });
    }

    // Connect toolbar actions to controllers
    QObject::connect(homeAction, &QAction::triggered,
                     &webPageController, &WebPageController::loadLocal);
    QObject::connect(remoteAction, &QAction::triggered,
                     &webPageController, &WebPageController::loadRemote);
    QObject::connect(backAction, &QAction::triggered,
                     &webPageController, &WebPageController::goBack);
    // Return focus to the page after a dialog closes, so the next tap on an
    // input field still raises the keyboard.
    // Return focus to the page once the dialog has fully closed. Doing it
    // immediately after exec() returns is too early: the dialog is still
    // unwinding and the focus it restores overrides ours, leaving the input
    // method pointed at a hidden dialog and the keyboard dead.
    auto refocusPage = [&webPageController, &window]() {
        QTimer::singleShot(0, webPageController.webView(), [&webPageController, &window]() {
            // Re-activate the main window, not just refocus the view.
            //
            // A dialog is a separate top-level window, and the input method
            // follows the ACTIVE WINDOW. The keyboard's InputPanel lives in
            // this window, so while a dialog is active the input context has
            // no panel to drive: showInputPanel() fires and nothing appears.
            // Restoring focus alone left the context bound to the closed
            // dialog's window, which is why the keyboard stayed dead after
            // opening Info once.
            window.raise();
            window.activateWindow();
            webPageController.webView()->setFocus();
        });
    };
    QObject::connect(wifiButton, &QToolButton::clicked,
                     [wifiDialog, refocusPage]() {
        DialogStyle::closeKeyboard();
        wifiDialog->exec();
        refocusPage();
    });
    QObject::connect(lanButton, &QToolButton::clicked,
                     [lanDialog, refocusPage]() {
        DialogStyle::closeKeyboard();
        lanDialog->exec();
        refocusPage();
    });
    QObject::connect(infoAction, &QAction::triggered,
                     [infoDialog, refocusPage]() {
        DialogStyle::closeKeyboard();
        infoDialog->exec();
        refocusPage();
    });

    // Wired state: shown only when the terminal has an ethernet port at all.
    // The radio can appear after startup — a USB adapter, or NetworkManager
    // taking longer than the browser to enumerate — so the button follows it
    // rather than being decided once.
    auto updateWifiVisible = [wifiButton, showWifi, &config, &networkController]() {
        wifiButton->setVisible(config.wifi() == AppConfig::On
                               || (showWifi
                                   && !networkController.wifiDevicePath().isEmpty()));
    };
    QObject::connect(&networkController, &NetworkController::connectedChanged,
                     updateWifiVisible);
    QObject::connect(&networkController, &NetworkController::networksChanged,
                     updateWifiVisible);

    auto updateLanIcon = [lanButton, showLan, &config, &networkController]() {
        // "on" keeps the button even with no device, so a terminal that is
        // supposed to be wired shows that it is not, rather than hiding the
        // problem.
        lanButton->setVisible(showLan
                              && (config.lan() == AppConfig::On
                                  || networkController.lanAvailable()));
        lanButton->setIcon(QIcon(networkController.lanConnected()
                                     ? ":/images/lan-up.png"
                                     : ":/images/lan-down.png"));
    };
    QObject::connect(&networkController, &NetworkController::lanChanged, updateLanIcon);

    auto updateWifiIcon = [wifiButton, &networkController]() {
        int level = networkController.signalLevel();
        if (level < 0)
            wifiButton->setIcon(QIcon(":/images/wifi-off.png"));
        else
            wifiButton->setIcon(QIcon(QString(":/images/wifi-%1.png").arg(level)));
    };
    QObject::connect(&networkController, &NetworkController::signalLevelChanged,
                     updateWifiIcon);

    // Seed both icons from the current state. NetworkController polls during
    // construction, before these connections exist, so its first — and on a
    // stable link, only — signal is emitted with nothing listening. Without
    // this the icons keep whatever they were built with: a wired terminal
    // showed "disconnected" indefinitely.
    updateLanIcon();
    updateWifiIcon();

    // Only wired up when the indicator exists — a desktop profile has none.
    if (scadaIndicator) {
    QObject::connect(scadaIndicator, &QToolButton::clicked,
                     [scadaIndicator, &window, refocusPage]() {
        DialogStyle::closeKeyboard();
        QDialog dialog(&window);
        dialog.setWindowTitle("SCADA Server");
        dialog.setStyleSheet("QDialog { background-color: #f0f0f0; }" + DialogStyle::sheet());
        DialogStyle::widthToScreen(&dialog, 0.94);

        auto* layout = new QVBoxLayout(&dialog);

        auto* headerRow = new QHBoxLayout;
        auto* head = new QLabel;
        head->setPixmap(RobotHead::pixmap(DialogStyle::px(56),
                                          scadaIndicator->variant()));
        headerRow->addWidget(head);
        auto* header = new QLabel("SCADA Server");
        header->setStyleSheet(QString("font-size: %1px; font-weight: bold;")
                                  .arg(DialogStyle::px(28)));
        headerRow->addWidget(header);
        headerRow->addStretch();
        layout->addLayout(headerRow);

        const QString station = scadaIndicator->station();
        const QString server = scadaIndicator->serverUrl();
        // The MAC is what the server identifies the terminal by, so it is what
        // someone provisioning it has to read off the screen and type in.
        const QString mac = scadaIndicator->macAddress();
        auto* body = new QLabel(
            "Status: " + scadaIndicator->statusText() + "\n"
            "Station: " + (station.isEmpty() ? QString("-") : station) + "\n"
            "MAC: " + (mac.isEmpty() ? QString("-") : mac) + "\n"
            "Server: " + (server.isEmpty() ? QString("-") : server));
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        body->setWordWrap(true);
        body->setStyleSheet(
            QString("font-family: monospace; font-size: %1px; background: white; ")
                .arg(DialogStyle::px(19)) +
            "border: 1px solid #ccc; border-radius: 6px; padding: 14px;");
        layout->addWidget(body);

        auto* row = new QHBoxLayout;
        row->addStretch();
        auto* closeBtn = new QPushButton("Close");
        closeBtn->setStyleSheet(DialogStyle::Colour::primary());
        QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        row->addWidget(closeBtn);
        layout->addLayout(row);

        DialogStyle::takeNoFocusExceptFields(&dialog);
        dialog.exec();
        refocusPage();
    });
    }

    QObject::connect(keyboardButton, &QToolButton::clicked,
                     [keyboard, &webPageController]() {
        if (!keyboard)
            return;
        if (keyboard->isShowing()) {
            // Blur whatever is focused on the page first.
            //
            // Hiding the panel while a text field still holds focus does not
            // stick: the field is an input-method client, Qt re-shows the panel
            // for it immediately, and the keyboard flickers and comes straight
            // back. Taking the focus away removes the thing that keeps asking.
            if (webPageController.webView() && webPageController.webView()->page()) {
                webPageController.webView()->page()->runJavaScript(
                    "if (document.activeElement && document.activeElement.blur)"
                    " document.activeElement.blur();");
            }
            keyboard->setForceVisible(false);
            QGuiApplication::inputMethod()->commit();
            QGuiApplication::inputMethod()->hide();
        } else {
            // Focus the page first: with the panel up and nothing focused,
            // keystrokes would go nowhere.
            webPageController.webView()->setFocus();
            keyboard->setForceVisible(true);
            QGuiApplication::inputMethod()->show();
        }
    });

    // Kiosk focus policy: web content owns keyboard focus, the toolbar never
    // takes it. Without this the first QToolButton holds focus from startup, so
    // the web view never becomes the input-method focus object and the virtual
    // keyboard is never raised for page input fields.
    for (QToolButton* button : toolbar->findChildren<QToolButton*>())
        button->setFocusPolicy(Qt::NoFocus);
    webPageController.webView()->setFocusPolicy(Qt::StrongFocus);

    // Any tap on the page reclaims keyboard focus for it. Without this, a
    // widget that takes focus and never gives it back leaves the on-screen
    // keyboard unable to appear for the rest of the session.
    app.installEventFilter(new PageFocusGuard(webPageController.webView(), &app));

    // Progress strip follows the load, and clears the offline notice when a
    // load actually starts.
    QObject::connect(&webPageController, &WebPageController::loadProgress,
                     [loadBar](int percent) {
        loadBar->setValue(percent);
    });
    QObject::connect(&webPageController, &WebPageController::loadingChanged,
                     [loadBar, offlineBanner, &webPageController]() {
        const bool loading = webPageController.loading();
        // Empty it at the start of a load and fill it at the end, rather than
        // showing and hiding the widget.
        loadBar->setValue(loading ? 0 : 100);
        if (loading)
            offlineBanner->hide();
    });

    // Offline notice hides itself: by the time an operator reads it and looks
    // up, it is usually stale.
    QTimer* offlineTimer = new QTimer(&window);
    offlineTimer->setSingleShot(true);
    offlineTimer->setInterval(8000);
    QObject::connect(offlineTimer, &QTimer::timeout, offlineBanner, &QLabel::hide);
    QObject::connect(&webPageController, &WebPageController::networkUnavailable,
                     [offlineBanner, offlineTimer]() {
        offlineBanner->show();
        offlineTimer->start();
    });

    // Load initial page. Remote is the landing page: the transaction page is
    // where operators spend their time, Home is the exception.
    // The page lays out at the device's size and is drawn smaller. Without
    // this the window would simply be a smaller viewport, which is the thing a
    // device profile exists to avoid.
    if (profile.scale < 1.0 && webPageController.webView())
        webPageController.webView()->setZoomFactor(profile.scale);

    webPageController.loadRemote();

    if (!profile.toolbar)
        toolbar->hide();

    QScreen* screen = app.primaryScreen();
    if (!profile.fullscreen) {
        const QRect avail = screen->availableGeometry();
        const QSize wanted = profile.windowSize.isValid()
            ? QSize(px(profile.windowSize.width()), px(profile.windowSize.height()))
            : profile.windowSize;
        if (wanted.isValid() && wanted.width() > 0 && wanted.height() > 0) {
            // A device profile is only useful at 1:1 — the whole point is that
            // a page overflowing the panel overflows here too. Warn rather
            // than silently shrink, so nobody trusts a viewport that is not
            // the device's.
            if (!avail.size().expandedTo(wanted).isNull()
                && (wanted.width() > avail.width() || wanted.height() > avail.height())) {
                fprintf(stderr,
                        "robot-browser: profile '%s' wants %dx%d but this screen "
                        "offers %dx%d — the window is not the device's viewport\n",
                        qPrintable(profile.name), wanted.width(), wanted.height(),
                        avail.width(), avail.height());
            }
            window.resize(wanted.boundedTo(avail.size()));
        } else {
            // Inset, so the window is obviously a window and its title bar —
            // and the close button on it — is reachable.
            window.resize(avail.width() * 9 / 10, avail.height() * 4 / 5);
        }
        window.show();
        if (qEnvironmentVariableIsSet("ROBOT_BROWSER_DEBUG_GEOMETRY")) {
            fprintf(stderr, "geometry: asked %dx%d, got %dx%d, minimumSizeHint %dx%d "
                            "(central %dx%d, toolbar %dx%d)\n",
                    wanted.width(), wanted.height(),
                    window.width(), window.height(),
                    window.minimumSizeHint().width(), window.minimumSizeHint().height(),
                    central->minimumSizeHint().width(), central->minimumSizeHint().height(),
                    toolbar->minimumSizeHint().width(), toolbar->minimumSizeHint().height());
        }
    } else {
        window.setGeometry(screen->geometry());
        window.showFullScreen();
    }
    webPageController.webView()->setFocus();
    if (keyboard) {
        // The window's width, not the screen's. On a terminal they are the
        // same thing; in a window they are not, and sizing the keyboard to the
        // screen gave the QQuickWidget a 1920px-wide root, which became the
        // window's minimum width — so a device profile could not be drawn at
        // the size it asked for.
        const int hostWidth  = profile.fullscreen ? screen->geometry().width()
                                                  : window.width();
        int hostHeight = profile.fullscreen ? screen->geometry().height()
                                            : window.height();
        // What the CENTRAL widget has, not what the screen has. The toolbar is
        // not part of it, and a side keyboard sized to the whole screen makes
        // the window taller than the screen: the compositor then un-fullscreens
        // it, which puts a title bar on the window and pushes the toolbar off
        // the bottom — both of which is exactly what happened.
        if (profile.toolbar && toolbar->isVisible())
            hostHeight -= toolbar->height();
        if (sideKeyboard) {
            // Half the width, and as tall as it likes: down the side the
            // constraint is horizontal, so the keys get the vertical room a
            // short panel could not give them at the bottom.
            keyboard->setSideDocked(hostWidth / 4, hostHeight);
        } else {
            // Four rows across about seventeen columns — far wider than the
            // ten-across layout's 0.465 — and allowed a larger share of the
            // screen, because it is the only page: there is no mode key to
            // press and nothing hidden behind one.
            // 22 "key widths" for the glyph scale, which is not the column
            // count — it is tuned so the letters are about 55% of the key's
            // HEIGHT.
            //
            // The style sizes text by width / keyboardDesignWidth, which is
            // right when keys are square. On this layout they are much wider
            // than tall — 66x44 on an ITPC-200 — so a width-derived glyph is
            // too tall for the key: at 19 the "." and "," were drawn past the
            // bottom edge, and at 28 (rows / aspect) everything was minute.
            // Measured on the hardware, between those.
            //
            // 0.14, well below the geometric 4/17, because height is the
            // scarce dimension and width is not. At the square ratio the keys
            // came out 35px wide and 44px tall on a 1920px panel with 1100px
            // of it unused. This spends that width on the keys: about 74px
            // each on an ITPC-200, and the full panel width on a T432.
            //
            // A third of the screen, the same as every other keyboard here.
            // It was given 45% on the grounds that it replaces three pages,
            // but half the view is half the view — the operator still has to
            // see what they are typing into.
            // How tall the keyboard may be in millimetres, not in pixels.
            //
            // Both bottom-docked layouts are four rows, and a touch key wants
            // about 11mm however big the display is — on an 18.5" ITPC-200 a
            // third of the screen gives 18mm keys and eats 73mm of panel.
            // Falls back to the pixel fraction where the screen reports no
            // physical size.
            int maxHeightPx = 0;
            const QSizeF physical = screen->physicalSize();
            if (physical.height() > 0 && screen->geometry().height() > 0) {
                const qreal mmPerPixel = physical.height()
                                         / screen->geometry().height();
                if (mmPerPixel > 0)
                    maxHeightPx = int(kRows * kKeyMillimetres / mmPerPixel);
            }

            keyboard->setPanelWidth(hostWidth, hostHeight,
                                    fullKeyboard ? 0.14 : 0.0,
                                    0.0,
                                    fullKeyboard ? 22 : 0,
                                    maxHeightPx);
        }
    }

    return app.exec();
}
