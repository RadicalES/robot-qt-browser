#include <QApplication>
#include <QMainWindow>
#include <QStackedLayout>
#include <QQuickWidget>
#include <QQmlContext>
#include <QWebSettings>
#include <QScreen>
#include <QDebug>

#include "overlayeventfilter.h"
#include "webpagecontroller.h"
#include "networkcontroller.h"
#include "systemcontroller.h"
#include "websockserver.h"
#include "unixsignalnotifier.h"

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

    // QML overlay (transparent, renders bar + popups + keyboard over webview)
    QQuickWidget* qmlOverlay = new QQuickWidget;
    qmlOverlay->setResizeMode(QQuickWidget::SizeRootObjectToView);
    qmlOverlay->setClearColor(Qt::transparent);
    qmlOverlay->setAttribute(Qt::WA_AlwaysStackOnTop);
    qmlOverlay->setAttribute(Qt::WA_TranslucentBackground);
    qmlOverlay->rootContext()->setContextProperty("webPageController", &webPageController);
    qmlOverlay->rootContext()->setContextProperty("networkController", &networkController);
    qmlOverlay->rootContext()->setContextProperty("systemController", &systemController);
    qmlOverlay->setSource(QUrl("qrc:/qml/main.qml"));

    // Event filter: forward mouse/keyboard from overlay to webview in pass-through area
    OverlayEventFilter* eventFilter = new OverlayEventFilter(qmlOverlay, webPageController.webView());
    qmlOverlay->installEventFilter(eventFilter);

    // Main window with stacked layout: webview underneath, QML overlay on top
    QMainWindow window;
    QWidget* central = new QWidget;
    QStackedLayout* stack = new QStackedLayout(central);
    stack->setStackingMode(QStackedLayout::StackAll);

    stack->addWidget(qmlOverlay);              // on top (transparent except bar/popups)
    stack->addWidget(webPageController.webView()); // underneath

    window.setCentralWidget(central);

    // Load initial page
    webPageController.loadRemote();

    // Show fullscreen for kiosk
    QScreen* screen = app.primaryScreen();
    window.setGeometry(screen->geometry());
    window.showFullScreen();

    return app.exec();
}
