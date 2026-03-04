#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QWebSettings>
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

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("Radical Electronic Systems");
    app.setApplicationName("RobotBrowser");
    app.setApplicationVersion("2.1");

    // Global WebKit settings
    QWebSettings::setMaximumPagesInCache(4);
    QWebSettings::setObjectCacheCapacities(128 * 1024, 1024 * 1024, 1024 * 1024);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, false);
    QWebSettings::enablePersistentStorage();

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

    // Main window — QWebView as central widget (receives native events directly)
    QMainWindow window;
    window.setCentralWidget(webPageController.webView());

    // Bottom toolbar (44px)
    QToolBar* toolbar = new QToolBar(&window);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(34, 34));
    toolbar->setFixedHeight(44);
    toolbar->setStyleSheet(
        "QToolBar { background: #2b2b2b; spacing: 4px; padding: 2px; border: none; }"
        "QToolButton { border: none; padding: 3px; }"
        "QToolButton:pressed { background: #555; border-radius: 3px; }");
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
    wifiButton->setIconSize(QSize(34, 34));
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
    QObject::connect(wifiButton, &QToolButton::clicked,
                     [wifiDialog]() { wifiDialog->exec(); });
    QObject::connect(infoAction, &QAction::triggered,
                     [infoDialog]() { infoDialog->exec(); });

    // Update WiFi icon when signal level changes
    QObject::connect(&networkController, &NetworkController::signalLevelChanged,
                     [wifiButton, &networkController]() {
        int level = networkController.signalLevel();
        if (level < 0)
            wifiButton->setIcon(QIcon(":/images/wifi-off.png"));
        else
            wifiButton->setIcon(QIcon(QString(":/images/wifi-%1.png").arg(level)));
    });

    // Load initial page
    webPageController.loadRemote();

    // Show fullscreen for kiosk
    QScreen* screen = app.primaryScreen();
    window.setGeometry(screen->geometry());
    window.showFullScreen();

    return app.exec();
}
