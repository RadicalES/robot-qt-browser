#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
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
#include "infodialog.h"
#include "virtualkeyboardpanel.h"
#include "pagefocusguard.h"
#include "dialogstyle.h"
#include "kioskstyle.h"

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

    // Parse URLs: robot-browser <remote_url> [local_url]
    QUrl localUrl("http://127.0.0.1");
    QUrl remoteUrl("http://127.0.0.1");

    QStringList args = app.arguments();
    if (args.size() > 1)
        remoteUrl = QUrl(args.at(1));
    if (args.size() > 2)
        localUrl = QUrl(args.at(2));

    // Unix signal handling for systemd
    UnixSignalNotifier::instance()->installSignalHandler(SIGINT);
    UnixSignalNotifier::instance()->installSignalHandler(SIGTERM);
    QObject::connect(UnixSignalNotifier::instance(), SIGNAL(unixSignal(int)),
                     &app, SLOT(quit()));

    // Debug WebSocket server
    WebsockServer debugSvr(7070, false, &app);

    // C++ backend controllers
    NetworkController networkController;
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
    centralLayout->addWidget(webPageController.webView(), 1);

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
    toolbar->setStyleSheet(
        "QToolBar { background: #4d4d4d; spacing: 10px; padding: 4px; border: none; }"
        "QToolButton { border: none; padding: 8px; min-width: 60px; }"
        "QToolButton:pressed { background: #808080; border-radius: 6px; }");
    window.addToolBar(Qt::BottomToolBarArea, toolbar);

    // Navigation buttons
    QAction* homeAction = toolbar->addAction(QIcon(":/images/home.png"), "");
    QAction* remoteAction = toolbar->addAction(QIcon(":/images/store.png"), "");
    QAction* backAction = toolbar->addAction(QIcon(":/images/back.png"), "");

    // Spacer
    QWidget* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // WiFi icon button
    QToolButton* wifiButton = new QToolButton;
    wifiButton->setIconSize(QSize(48, 48));
    wifiButton->setAutoRaise(true);
    wifiButton->setIcon(QIcon(":/images/wifi-off.png"));
    toolbar->addWidget(wifiButton);

    // Clock
    DigitalClock* clock = new DigitalClock;
    toolbar->addWidget(clock);

    // Info button
    QAction* infoAction = toolbar->addAction(QIcon(":/images/info.png"), "");

    // Dialogs
    WifiDialog* wifiDialog = new WifiDialog(&networkController, &window);
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
    QObject::connect(infoAction, &QAction::triggered,
                     [infoDialog, refocusPage]() {
        DialogStyle::closeKeyboard();
        infoDialog->exec();
        refocusPage();
    });

    // Update WiFi icon when signal level changes
    QObject::connect(&networkController, &NetworkController::signalLevelChanged,
                     [wifiButton, &networkController]() {
        int level = networkController.signalLevel();
        if (level < 0)
            wifiButton->setIcon(QIcon(":/images/wifi-off.png"));
        else
            wifiButton->setIcon(QIcon(QString(":/images/wifi-%1.png").arg(level)));
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

    // Load initial page
    webPageController.loadRemote();

    // Show fullscreen for kiosk
    QScreen* screen = app.primaryScreen();
    window.setGeometry(screen->geometry());
    window.showFullScreen();
    webPageController.webView()->setFocus();
    keyboard->setPanelWidth(screen->geometry().width());

    return app.exec();
}
