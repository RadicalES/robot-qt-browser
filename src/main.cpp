#include <QApplication>
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
#include <QScreen>
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
#include "robothead.h"
#include "dialogstyle.h"
#include "kioskstyle.h"
#include "appconfig.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    // Raise the keyboard on the FIRST tap of an input, not the second.
    app.setStyle(new KioskStyle);
    app.setOrganizationName("Radical Electronic Systems");
    app.setApplicationName("RobotBrowser");
    app.setApplicationVersion("2.1");

    // Engine settings and persistent storage are configured per-profile in
    // WebPageController — QtWebEngine has no equivalent of QWebSettings globals.

    // Settings: built-in defaults, then the package's own config file, then the
    // command line. The package is installed standalone from the CDN, so it
    // runs from the file alone; a provisioning layer integrates by passing what
    // it holds as arguments, without this needing to know that layer exists.
    //
    //   robot-browser [--config=PATH] [--wifi=auto|on|off] [--lan=...]
    //                 [--windowed[=WxH]] [--no-toolbar] [remote_url] [local_url]
    AppConfig config;
    QStringList positional;
    // Kiosk by default: fullscreen, toolbar, the whole panel. The flags below
    // exist for the other case — the browser opened from a desktop session as a
    // single-purpose tool (the WiFi setup page, say), where a fullscreen window
    // with no exit traps whoever opened it and the kiosk toolbar means nothing.
    //
    //   --windowed[=WxH]   a normal window the compositor can decorate and close
    //   --no-toolbar       drop the bottom bar; the page is the whole point
    bool windowed = false;
    bool showToolbar = true;
    QSize windowSize;
    QString configPath = AppConfig::defaultPath();
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i);
        if (arg.startsWith("--config="))
            configPath = arg.mid(strlen("--config="));
        else if (arg == "--windowed" || arg.startsWith("--windowed=")) {
            windowed = true;
            if (arg.contains('=')) {
                const QStringList wh = arg.section('=', 1).split('x');
                if (wh.size() == 2)
                    windowSize = QSize(wh.at(0).toInt(), wh.at(1).toInt());
            }
        }
        else if (arg == "--no-toolbar")
            showToolbar = false;
        else if (!arg.startsWith("--"))
            positional << arg;
    }
    config.loadFile(configPath);
    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i).startsWith("--"))
            config.applyArgument(args.at(i));
    }
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
    QWidget* central = new QWidget;
    QVBoxLayout* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    // Offline notice: an inline strip rather than a modal. A packhouse dead
    // zone is routine, and a dialog would block the operator and steal focus
    // from the page for something that fixes itself when they move.
    QLabel* offlineBanner = new QLabel(
        "No network — the page could not be loaded. It will work again once "
        "the terminal is back in range.");
    offlineBanner->setWordWrap(true);
    offlineBanner->setStyleSheet(
        "background: #ffa726; color: #4d4d4d; font-size: 20px; padding: 14px;");
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

    VirtualKeyboardPanel* keyboard = new VirtualKeyboardPanel;
    centralLayout->addWidget(keyboard);

    window.setCentralWidget(central);

    // Bottom toolbar. Sized for finger use on a touch-only terminal: 44px
    // with 34px icons was too small a target to hit reliably.
    QToolBar* toolbar = new QToolBar(&window);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(48, 48));
    toolbar->setFixedHeight(76);
    // Tight spacing: with WiFi, LAN and the SCADA head all present the row is
    // nine buttons plus the clock, which at a comfortable 10px gap and 60px
    // buttons overflows a 720px panel. The 48px icons still give a finger-sized
    // target on their own, so the padding around them is what gives way.
    toolbar->setStyleSheet(
        "QToolBar { background: #4d4d4d; spacing: 2px; padding: 2px; border: none; }"
        "QToolButton { border: none; padding: 3px; min-width: 54px; }"
        "QToolButton:pressed { background: #808080; border-radius: 6px; }");
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
    wifiButton->setIconSize(QSize(48, 48));
    wifiButton->setAutoRaise(true);
    wifiButton->setIcon(QIcon(":/images/wifi-off.png"));
    wifiButton->setVisible(showWifi);
    toolbar->addWidget(wifiButton);

    // LAN icon button, beside WiFi: the T430 terminals run wired, so the
    // operator needs the same at-a-glance state and the same way in to
    // settings for either link.
    QToolButton* lanButton = new QToolButton;
    lanButton->setIconSize(QSize(48, 48));
    lanButton->setAutoRaise(true);
    lanButton->setIcon(QIcon(":/images/lan-down.png"));
    lanButton->setVisible(showLan && networkController.lanAvailable());
    toolbar->addWidget(lanButton);

    // SCADA server status, mirroring the desktop tray indicator: grey when no
    // server or service is detected, orange when the terminal is not
    // provisioned, green when it is talking to a server.
    ScadaIndicator* scadaIndicator = new ScadaIndicator;
    toolbar->addWidget(scadaIndicator);

    // Manual keyboard toggle. The keyboard normally follows the focused field,
    // but an operator needs a way to dismiss it while reading, and a way to
    // raise it if it fails to appear.
    QToolButton* keyboardButton = new QToolButton;
    keyboardButton->setIconSize(QSize(48, 48));
    keyboardButton->setAutoRaise(true);
    keyboardButton->setIcon(QIcon(":/images/keyboard.png"));
    toolbar->addWidget(keyboardButton);

    // Clock
    DigitalClock* clock = new DigitalClock;
    toolbar->addWidget(clock);

    // Info button
    QAction* infoAction = toolbar->addAction(QIcon(":/images/info.png"), "");

    // Dialogs
    WifiDialog* wifiDialog = new WifiDialog(&networkController, &window);
    LanDialog* lanDialog = new LanDialog(&networkController, &window);
    InfoDialog* infoDialog = new InfoDialog(&systemController, &window);

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
                     [infoDialog, scadaIndicator, refocusPage]() {
        DialogStyle::closeKeyboard();
        infoDialog->setStatus(scadaIndicator->variant());
        infoDialog->exec();
        refocusPage();
    });

    // Wired state: shown only when the terminal has an ethernet port at all.
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
        head->setPixmap(RobotHead::pixmap(56, scadaIndicator->variant()));
        headerRow->addWidget(head);
        auto* header = new QLabel("SCADA Server");
        header->setStyleSheet("font-size: 28px; font-weight: bold;");
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
            "font-family: monospace; font-size: 19px; background: white; "
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

    QObject::connect(keyboardButton, &QToolButton::clicked,
                     [keyboard, &webPageController]() {
        if (keyboard->isShowing()) {
            keyboard->setForceVisible(false);
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
    webPageController.loadRemote();

    if (!showToolbar)
        toolbar->hide();

    QScreen* screen = app.primaryScreen();
    if (windowed) {
        const QRect avail = screen->availableGeometry();
        if (windowSize.isValid() && windowSize.width() > 0 && windowSize.height() > 0) {
            window.resize(windowSize.boundedTo(avail.size()));
        } else {
            // Inset, so the window is obviously a window and its title bar —
            // and the close button on it — is reachable.
            window.resize(avail.width() * 9 / 10, avail.height() * 4 / 5);
        }
        window.show();
    } else {
        window.setGeometry(screen->geometry());
        window.showFullScreen();
    }
    webPageController.webView()->setFocus();
    keyboard->setPanelWidth(screen->geometry().width());

    return app.exec();
}
