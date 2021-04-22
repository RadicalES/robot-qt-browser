/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 University of Szeged
 * Copyright (C) 2011 Kristof Kosztyo <Kosztyo.Kristof@stud.u-szeged.hu>
 * Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "launcherwindow.h"
#include "cookiejar.h"
#include "urlloader.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QLabel>
#ifndef QT_NO_LINEEDIT
#include <QLineEdit>
#endif
#ifndef QT_NO_SHORTCUT
#include <QMenuBar>
#endif
#if !defined(QT_NO_PRINTPREVIEWDIALOG) && HAVE(QTPRINTSUPPORT)
#include <QPrintPreviewDialog>
#endif
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QQuickWidget>
//#include <QtVirtualKeyboard/qvirtualkeyboardinputcontext.h>
#include <QVirtualKeyboardInputEngine>

#if !defined(QT_NO_FILEDIALOG) && !defined(QT_NO_MESSAGEBOX)
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkReply>
#endif

#if !defined(QT_NO_NETWORKDISKCACHE) && !defined(QT_NO_DESKTOPSERVICES)
#include <QStandardPaths>
#include <QtNetwork/QNetworkDiskCache>
#endif

struct HighlightedElement {
    QWebElement m_element;
    QString m_previousStyle;
};

const int gExitClickArea = 80;
QVector<int> LauncherWindow::m_zoomLevels;

static TestBrowserCookieJar* testBrowserCookieJarInstance()
{
    static TestBrowserCookieJar* cookieJar = new TestBrowserCookieJar(qApp);
    return cookieJar;
}

LauncherWindow::LauncherWindow(WindowOptions* data, QGraphicsScene* sharedScene)
    : MainWindow(data->screenGeometry.width() > data->screenGeometry.height())
    , m_currentZoom(100)
    , m_urlLoader(nullptr)
    , m_view(nullptr)
    , m_inspector(nullptr)
    , m_formatMenuAction(nullptr)
    , m_zoomAnimation(nullptr)    
    , m_eventController(nullptr)
    , m_keyboard(new KeyboardWidget(this))
{
    if (data) {
        m_windowOptions = *data;
        setLocalURL(m_windowOptions.localUrl);
        setRemoteURL(m_windowOptions.remoteUrl);
        setImagesDir(m_windowOptions.appDir);
    }

    m_keyboard->setFocusPolicy(Qt::NoFocus);
    m_keyboard->setProperty("width", m_windowOptions.screenGeometry.width());



    m_keyboard->setSource(QUrl::fromLocalFile("keyboard.qml"));
    m_keyboard->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_keyboard->setVisible(false);
    m_keyboard->setAttribute(Qt::WA_AcceptTouchEvents, true); // solve touch event bug

    keyboardEnabled = true;
    inputPanel = (QObject *)m_keyboard->rootObject();
    if (inputPanel) {
            inputPanel->setProperty("width", m_windowOptions.screenGeometry.width());
            if(m_windowOptions.screenGeometry.width() == 800) {
                inputPanel->setProperty("height", 190);
                inputPanel->setProperty("designHeight", 600);
            }
            else {
                inputPanel->setProperty("height", 300);
                inputPanel->setProperty("designHeight", 1600);
            }
    }

    if(m_windowOptions.screenGeometry.width() == 800) {
        m_keyboard->setProperty("height", 190);
    }
    else {
        m_keyboard->setProperty("height", 300);
    }

    connect(inputPanel, SIGNAL(activated(bool)), this, SLOT(onKeyboardActiveChanged(bool)));
    connect(inputPanel, SIGNAL(heightChanged(int)), this, SLOT(onKeyboardHeightChanged(int)));

    init();
    if (sharedScene && data->useGraphicsView)
        static_cast<QGraphicsView*>(m_view)->setScene(sharedScene);

#if !defined(QT_NO_FILEDIALOG) && !defined(QT_NO_MESSAGEBOX)
    connect(page(), SIGNAL(downloadRequested(const QNetworkRequest&)), this, SLOT(downloadRequest(const QNetworkRequest&)));
    connect(page()->networkAccessManager(), SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(showSSLErrorConfirmation(QNetworkReply*, const QList<QSslError>&)));
#endif
}

LauncherWindow::~LauncherWindow()
{
    delete m_urlLoader;
}

void LauncherWindow::createEvents()
{
    QObject::connect(&m_eventController, &EventController::onKeyboardShowRequest, [this](void) {
        onKeyboardActiveChanged(true);
    });

    QObject::connect(&m_eventController, &EventController::onKeyboardHideRequest, [this](void) {
        onKeyboardActiveChanged(false);
    });

    QObject::connect(&m_eventController, &EventController::onSetKeyboardEnabledRequested, [this](bool enabled){
        if (!enabled)
            m_keyboard->setVisible(false);
        this->keyboardEnabled = enabled;
    });

    QObject::connect(&m_eventController, &EventController::onKeyboardShown, [this](int height){
//            m_keyboard->setVisible(false);
  //      this->keyboardEnabled = enabled;
   //    qDebug("onKeyboardShown");
      //  QVariant returnedValue;
       // QVariant *h = new QVariant(height);
       // QMetaObject::invokeMethod(inputPanel, "setHeight",
            //    Q_RETURN_ARG(QVariant, returnedValue),
            //    Q_ARG(QVariant, *h));
      //  delete h;
    });
}

void LauncherWindow::init()
{
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(splitter);

    // force window to screen width and set maximized
    resize(m_windowOptions.screenGeometry.width(), m_windowOptions.screenGeometry.height());
    setWindowState(windowState() | Qt::WindowMaximized);

    m_inspector = new WebInspector;
#ifndef QT_NO_PROPERTIES
    if (!m_windowOptions.inspectorUrl.isEmpty())
        m_inspector->setProperty("_q_inspectorUrl", m_windowOptions.inspectorUrl);
#endif
    connect(this, SIGNAL(destroyed()), m_inspector, SLOT(deleteLater()));

    // the zoom values are chosen to be like in Mozilla Firefox 3
    if (!m_zoomLevels.count()) {
        m_zoomLevels << 30 << 50 << 67 << 80 << 90;
        m_zoomLevels << 100;
        m_zoomLevels << 110 << 120 << 133 << 150 << 170 << 200 << 240 << 300;
    }

    initializeView();    
}

void LauncherWindow::initializeView()
{
    delete m_view;

    m_inputUrl = addressUrl();
    QUrl url = page()->mainFrame()->url();
    setPage(new WebPage(this));
    setDiskCache(m_windowOptions.useDiskCache);
    setUseDiskCookies(m_windowOptions.useDiskCookies);

    // We reuse the same cookieJar on multiple QNAMs, which is OK.
    QObject* cookieJarParent = testBrowserCookieJarInstance()->parent();
    page()->networkAccessManager()->setCookieJar(testBrowserCookieJarInstance());
    testBrowserCookieJarInstance()->setParent(cookieJarParent);

    QSplitter* splitter = static_cast<QSplitter*>(centralWidget());

    if (!m_windowOptions.useGraphicsView) {
        WebViewTraditional* view = new WebViewTraditional(splitter);
        view->setPage(page());

        view->installEventFilter(this);

        m_view = view;
    } else {
        WebViewGraphicsBased* view = new WebViewGraphicsBased(splitter);
        m_view = view;
        if (!m_windowOptions.useQOpenGLWidgetViewport)
            toggleQGLWidgetViewport(m_windowOptions.useQGLWidgetViewport);
#ifdef QT_OPENGL_LIB
        if (!m_windowOptions.useQGLWidgetViewport)
            toggleQOpenGLWidgetViewport(m_windowOptions.useQOpenGLWidgetViewport);
#endif
        view->setPage(page());

        connect(view, SIGNAL(currentFPSUpdated(int)), this, SLOT(updateFPS(int)));

        view->installEventFilter(this);
        // The implementation of QAbstractScrollArea::eventFilter makes us need
        // to install the event filter also on the viewport of a QGraphicsView.
        view->viewport()->installEventFilter(this);
    }

    m_touchMocking = false;

    connect(page(), SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    connect(page(), SIGNAL(loadFinished(bool)), this, SLOT(loadFinished()));
    connect(this, SIGNAL(enteredFullScreenMode(bool)), this, SLOT(toggleFullScreenMode(bool)));

    if (m_windowOptions.printLoadedUrls)
        connect(page()->mainFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(printURL(QUrl)));

    applyPrefs();

    //QWidget* urlEdit = static_cast<QWidget*>(m_urlEdit);

    //if(!m_landscape) {
       // splitter->addWidget((QWidget *)m_urlEdit);
   // }
    splitter->addWidget(m_keyboard);
//    QList<int> Sizes;
//    Sizes.append(600);
//    Sizes.append(30);
//    Sizes.append(120);
//    splitter->setSizes(Sizes);
//    splitter->setStretchFactor(0,1);
//    splitter->setStretchFactor(1,0);
//    splitter->setStretchFactor(2,0);


    //splitter->addWidget(m_inspector);
       //addWidget(m_keyboard);

    m_inspector->setPage(page());
    m_inspector->hide();

    if (m_windowOptions.remoteInspectorPort)
        page()->setProperty("_q_webInspectorServerPort", m_windowOptions.remoteInspectorPort);

    if (url.isValid())
        page()->mainFrame()->load(url);
    else  {
        setAddressUrl(m_inputUrl);
        m_inputUrl = QString();
    }
}

void LauncherWindow::applyPrefs()
{
    QWebSettings* settings = page()->settings();

#ifndef QT_NO_OPENGL
    settings->setAttribute(QWebSettings::AcceleratedCompositingEnabled, m_windowOptions.useCompositing
        && (m_windowOptions.useQGLWidgetViewport || m_windowOptions.useQOpenGLWidgetViewport));
#endif

    settings->setAttribute(QWebSettings::TiledBackingStoreEnabled, m_windowOptions.useTiledBackingStore);
    settings->setAttribute(QWebSettings::FrameFlatteningEnabled, m_windowOptions.useFrameFlattening);
    settings->setAttribute(QWebSettings::WebGLEnabled, m_windowOptions.useWebGL);
    settings->setAttribute(QWebSettings::MediaEnabled, m_windowOptions.useMedia);
    m_windowOptions.useWebAudio = settings->testAttribute(QWebSettings::WebAudioEnabled);
    m_windowOptions.useMediaSource = settings->testAttribute(QWebSettings::MediaSourceEnabled);

    settings->setAttribute(QWebSettings::PluginsEnabled, true);

    if (!isGraphicsBased())
        return;

    WebViewGraphicsBased* view = static_cast<WebViewGraphicsBased*>(m_view);
    view->setViewportUpdateMode(m_windowOptions.viewportUpdateMode);
    view->setFrameRateMeasurementEnabled(m_windowOptions.showFrameRate);
    view->setItemCacheMode(m_windowOptions.cacheWebView ? QGraphicsItem::DeviceCoordinateCache : QGraphicsItem::NoCache);

    if (m_windowOptions.resizesToContents)
        toggleResizesToContents(m_windowOptions.resizesToContents);
}

bool LauncherWindow::isGraphicsBased() const
{
    return bool(qobject_cast<QGraphicsView*>(m_view));
}

void LauncherWindow::closeEvent(QCloseEvent* e)
{
    e->ignore();
    auto c = connect(page(), &QWebPage::windowCloseRequested, this, [e]() {
        e->accept();
    });
    page()->triggerAction(QWebPage::RequestClose);
    disconnect(c);
}

void LauncherWindow::sendTouchEvent()
{
    if (m_touchPoints.isEmpty())
        return;

  //  qDebug("sendTouchEvent");

    QEvent::Type type = QEvent::TouchUpdate;
    if (m_touchPoints.size() == 1) {
        if (m_touchPoints[0].state() == Qt::TouchPointReleased)
            type = QEvent::TouchEnd;
        else if (m_touchPoints[0].state() == Qt::TouchPointPressed)
            type = QEvent::TouchBegin;
    }

    QTouchEvent touchEv(type);
    touchEv.setTouchPoints(m_touchPoints);
    QCoreApplication::sendEvent(page(), &touchEv);

    // After sending the event, remove all touchpoints that were released
    if (m_touchPoints[0].state() == Qt::TouchPointReleased)
        m_touchPoints.removeAt(0);
    if (m_touchPoints.size() > 1 && m_touchPoints[1].state() == Qt::TouchPointReleased)
        m_touchPoints.removeAt(1);
}

bool LauncherWindow::eventFilter(QObject* obj, QEvent* event)
{
   // qDebug("eventFilter");

    // If click pos is the bottom right corner (square with size defined by gExitClickArea)
    // and the window is on FullScreen, the window must return to its original state.
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* ev = static_cast<QMouseEvent*>(event);
        if (windowState() == Qt::WindowFullScreen
            && ev->pos().x() > (width() - gExitClickArea)
            && ev->pos().y() > (height() - gExitClickArea)) {

            emit enteredFullScreenMode(false);
        }
    }

    if (!m_touchMocking)
        return QObject::eventFilter(obj, event);

    //case QEvent::KeyPress:
    //case QEvent::KeyRelease:

    if (event->type() == QEvent::MouseButtonPress
        || event->type() == QEvent::MouseButtonRelease
        || event->type() == QEvent::MouseButtonDblClick
        || event->type() == QEvent::MouseMove) {

        QMouseEvent* ev = static_cast<QMouseEvent*>(event);
        if (ev->type() == QEvent::MouseMove
            && !(ev->buttons() & Qt::LeftButton))
            return false;

        QTouchEvent::TouchPoint touchPoint;
        touchPoint.setState(Qt::TouchPointMoved);
        if ((ev->type() == QEvent::MouseButtonPress
             || ev->type() == QEvent::MouseButtonDblClick))
            touchPoint.setState(Qt::TouchPointPressed);
        else if (ev->type() == QEvent::MouseButtonRelease)
            touchPoint.setState(Qt::TouchPointReleased);

        touchPoint.setId(0);
        touchPoint.setScreenPos(ev->globalPos());
        touchPoint.setPos(ev->pos());
        touchPoint.setPressure(1);

        // If the point already exists, update it. Otherwise create it.
        if (m_touchPoints.size() > 0 && !m_touchPoints[0].id())
            m_touchPoints[0] = touchPoint;
        else if (m_touchPoints.size() > 1 && !m_touchPoints[1].id())
            m_touchPoints[1] = touchPoint;
        else
            m_touchPoints.append(touchPoint);

        sendTouchEvent();
    } else if (event->type() == QEvent::KeyPress
        && static_cast<QKeyEvent*>(event)->key() == Qt::Key_F
        && static_cast<QKeyEvent*>(event)->modifiers() == Qt::ControlModifier) {

        // If the keyboard point is already pressed, release it.
        // Otherwise create it and append to m_touchPoints.
        if (m_touchPoints.size() > 0 && m_touchPoints[0].id() == 1) {
            m_touchPoints[0].setState(Qt::TouchPointReleased);
            sendTouchEvent();
        } else if (m_touchPoints.size() > 1 && m_touchPoints[1].id() == 1) {
            m_touchPoints[1].setState(Qt::TouchPointReleased);
            sendTouchEvent();
        } else {
            QTouchEvent::TouchPoint touchPoint;
            touchPoint.setState(Qt::TouchPointPressed);
            touchPoint.setId(1);
            touchPoint.setScreenPos(QCursor::pos());
            touchPoint.setPos(m_view->mapFromGlobal(QCursor::pos()));
            touchPoint.setPressure(1);
            m_touchPoints.append(touchPoint);
            sendTouchEvent();

            // After sending the event, change the touchpoint state to stationary
            m_touchPoints.last().setState(Qt::TouchPointStationary);
        }
    }

    return false;
}

void LauncherWindow::loadStarted()
{
    m_view->setFocus(Qt::OtherFocusReason);
}

void LauncherWindow::loadFinished()
{
    QUrl url = page()->mainFrame()->url();
    addCompleterEntry(url);
    if (m_inputUrl.isEmpty())
        setAddressUrl(url.toString(QUrl::RemoveUserInfo));
    else {
        setAddressUrl(m_inputUrl);
        m_inputUrl = QString();
    }        
}

void LauncherWindow::zoomAnimationFinished()
{
    if (!isGraphicsBased())
        return;
    QGraphicsWebView* view = static_cast<WebViewGraphicsBased*>(m_view)->graphicsWebView();
    view->setTiledBackingStoreFrozen(false);
}

void LauncherWindow::applyZoom()
{
#ifndef QT_NO_ANIMATION
    if (isGraphicsBased() && page()->settings()->testAttribute(QWebSettings::TiledBackingStoreEnabled)) {
        QGraphicsWebView* view = static_cast<WebViewGraphicsBased*>(m_view)->graphicsWebView();
        view->setTiledBackingStoreFrozen(true);
        if (!m_zoomAnimation) {
            m_zoomAnimation = new QPropertyAnimation(view, "scale");
            m_zoomAnimation->setStartValue(view->scale());
            connect(m_zoomAnimation, SIGNAL(finished()), this, SLOT(zoomAnimationFinished()));
        } else {
            m_zoomAnimation->stop();
            m_zoomAnimation->setStartValue(m_zoomAnimation->currentValue());
        }

        m_zoomAnimation->setDuration(300);
        m_zoomAnimation->setEndValue(qreal(m_currentZoom) / 100.);
        m_zoomAnimation->start();
        return;
    }
#endif
    page()->mainFrame()->setZoomFactor(qreal(m_currentZoom) / 100.0);
}

void LauncherWindow::zoomIn()
{
    int i = m_zoomLevels.indexOf(m_currentZoom);
    Q_ASSERT(i >= 0);
    if (i < m_zoomLevels.count() - 1)
        m_currentZoom = m_zoomLevels[i + 1];

    applyZoom();
}

void LauncherWindow::zoomOut()
{
    int i = m_zoomLevels.indexOf(m_currentZoom);
    Q_ASSERT(i >= 0);
    if (i > 0)
        m_currentZoom = m_zoomLevels[i - 1];

    applyZoom();
}

void LauncherWindow::resetZoom()
{
    m_currentZoom = 100;
    applyZoom();
}

void LauncherWindow::toggleZoomTextOnly(bool b)
{
    page()->settings()->setAttribute(QWebSettings::ZoomTextOnly, b);
}


/*
void LauncherWindow::dumpPlugins() {
    QList<QWebPluginInfo> plugins = QWebSettings::pluginDatabase()->plugins();
    foreach (const QWebPluginInfo plugin, plugins) {
        qDebug() << "Plugin:" << plugin.name();
        foreach (const QWebPluginInfo::MimeType mime, plugin.mimeTypes()) {
            qDebug() << "   " << mime.name;
        }
    }
}
*/

void LauncherWindow::dumpHtml()
{
    qDebug() << "HTML: " << page()->mainFrame()->toHtml();
}

void LauncherWindow::setDiskCache(bool enable)
{
#if !defined(QT_NO_NETWORKDISKCACHE) && !defined(QT_NO_DESKTOPSERVICES)
    m_windowOptions.useDiskCache = enable;
    QNetworkDiskCache* cache = 0;
    if (enable) {
        cache = new QNetworkDiskCache();
        QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        cache->setCacheDirectory(cacheLocation);
    }
    page()->networkAccessManager()->setCache(cache);
#endif
}

void LauncherWindow::setTouchMocking(bool on)
{
    m_touchMocking = on;
}

void LauncherWindow::toggleWebView(bool graphicsBased)
{
    m_windowOptions.useGraphicsView = graphicsBased;
    initializeView();
#ifndef QT_NO_SHORTCUT
  //  menuBar()->clear();
#endif
  //  createChrome();
}

void LauncherWindow::toggleAcceleratedCompositing(bool toggle)
{
    m_windowOptions.useCompositing = toggle;
    page()->settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, toggle);
}

void LauncherWindow::toggleAccelerated2dCanvas(bool toggle)
{
    page()->settings()->setAttribute(QWebSettings::Accelerated2dCanvasEnabled, toggle);
}

void LauncherWindow::toggleTiledBackingStore(bool toggle)
{
    page()->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, toggle);
}

void LauncherWindow::toggleResizesToContents(bool toggle)
{
    m_windowOptions.resizesToContents = toggle;
    static_cast<WebViewGraphicsBased*>(m_view)->setResizesToContents(toggle);
}

void LauncherWindow::toggleWebGL(bool toggle)
{
    m_windowOptions.useWebGL = toggle;
    page()->settings()->setAttribute(QWebSettings::WebGLEnabled, toggle);
}

void LauncherWindow::toggleMedia(bool toggle)
{
    m_windowOptions.useMedia = toggle;
    page()->settings()->setAttribute(QWebSettings::MediaEnabled, toggle);
}

void LauncherWindow::toggleWebAudio(bool toggle)
{
    m_windowOptions.useWebAudio = toggle;
    page()->settings()->setAttribute(QWebSettings::WebAudioEnabled, toggle);
}

void LauncherWindow::toggleMediaSource(bool toggle)
{
    m_windowOptions.useMediaSource = toggle;
    page()->settings()->setAttribute(QWebSettings::MediaSourceEnabled, toggle);
}

void LauncherWindow::animatedFlip()
{
    qobject_cast<WebViewGraphicsBased*>(m_view)->animatedFlip();
}

void LauncherWindow::animatedYFlip()
{
    qobject_cast<WebViewGraphicsBased*>(m_view)->animatedYFlip();
}

void LauncherWindow::toggleSpatialNavigation(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::SpatialNavigationEnabled, enable);
}

void LauncherWindow::toggleCaretBrowsing(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::CaretBrowsingEnabled, enable);
}

void LauncherWindow::toggleFullScreenMode(bool enable)
{
    bool alreadyEnabled = windowState() & Qt::WindowFullScreen;
    if (enable ^ alreadyEnabled)
        setWindowState(windowState() ^ Qt::WindowFullScreen);
}

void LauncherWindow::toggleFrameFlattening(bool toggle)
{
    m_windowOptions.useFrameFlattening = toggle;
    page()->settings()->setAttribute(QWebSettings::FrameFlatteningEnabled, toggle);
}

void LauncherWindow::toggleJavaScriptEnabled(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, enable);
}

void LauncherWindow::toggleInterruptingJavaScriptEnabled(bool enable)
{
    page()->setInterruptingJavaScriptEnabled(enable);
}

void LauncherWindow::toggleJavascriptCanOpenWindows(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, enable);
}

void LauncherWindow::togglePrivateBrowsing(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, enable);
}

void LauncherWindow::toggleWebSecurity(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::WebSecurityEnabled, !enable);
}

void LauncherWindow::setUseDiskCookies(bool enable)
{
    testBrowserCookieJarInstance()->setDiskStorageEnabled(enable);
}

void LauncherWindow::clearCookies()
{
    testBrowserCookieJarInstance()->reset();
}

void LauncherWindow::toggleAutoLoadImages(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::AutoLoadImages, !enable);
}

void LauncherWindow::togglePlugins(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::PluginsEnabled, !enable);
}

#ifndef QT_NO_OPENGL
void LauncherWindow::toggleQGLWidgetViewport(bool enable)
{
    if (!isGraphicsBased())
        return;

    if (enable)
        m_windowOptions.useQOpenGLWidgetViewport = false;
    m_windowOptions.useQGLWidgetViewport = enable;

    WebViewGraphicsBased* view = static_cast<WebViewGraphicsBased*>(m_view);
    view->setViewport(enable ? new QGLWidget() : 0);
}

void LauncherWindow::toggleQOpenGLWidgetViewport(bool enable)
{
    if (!isGraphicsBased())
        return;

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    if (enable)
        m_windowOptions.useQGLWidgetViewport = false;
    m_windowOptions.useQOpenGLWidgetViewport = enable;

    WebViewGraphicsBased* view = static_cast<WebViewGraphicsBased*>(m_view);
    view->setViewport(enable ? new QOpenGLWidget() : 0);
#endif
}
#endif

void LauncherWindow::changeViewportUpdateMode(int mode)
{
    m_windowOptions.viewportUpdateMode = QGraphicsView::ViewportUpdateMode(mode);

    if (!isGraphicsBased())
        return;

    WebViewGraphicsBased* view = static_cast<WebViewGraphicsBased*>(m_view);
    view->setViewportUpdateMode(m_windowOptions.viewportUpdateMode);
}

void LauncherWindow::showUserAgentDialog()
{
    QStringList items;
    QFile file(":/useragentlist.txt");
    if (file.open(QIODevice::ReadOnly)) {
         while (!file.atEnd())
            items << file.readLine().trimmed();
        file.close();
    }

    QSettings settings;
    QString customUserAgent = settings.value("CustomUserAgent").toString();
    if (!items.contains(customUserAgent) && !customUserAgent.isEmpty())
        items << customUserAgent;

    QDialog* dialog = new QDialog(this);
    dialog->resize(size().width() * 0.7, dialog->size().height());
    dialog->setWindowTitle("Change User Agent");

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    dialog->setLayout(layout);

#ifndef QT_NO_COMBOBOX
    QComboBox* combo = new QComboBox(dialog);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    combo->setEditable(true);
    combo->insertItems(0, items);
    layout->addWidget(combo);

    int index = combo->findText(page()->userAgentForUrl(QUrl()));
    combo->setCurrentIndex(index);
#endif

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
            | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
    connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));
    layout->addWidget(buttonBox);

#ifndef QT_NO_COMBOBOX
    if (dialog->exec() && !combo->currentText().isEmpty()) {
        page()->setUserAgent(combo->currentText());
        if (!items.contains(combo->currentText()))
            settings.setValue("CustomUserAgent", combo->currentText());
    }
#endif

    delete dialog;
}

void LauncherWindow::showSSLErrorConfirmation(QNetworkReply* reply, const QList<QSslError>& errors)
{
    QString errorStrings = "<ul>";
    for (const QSslError& error : errors)
        errorStrings += "<li>" + error.errorString() + "</li>";
    errorStrings += "</ul>";

    QMessageBox sslWarningBox;
    sslWarningBox.setText("TLS handshake problem");
    sslWarningBox.setInformativeText(errorStrings);
    sslWarningBox.setStandardButtons(QMessageBox::Abort | QMessageBox::Ignore);
    sslWarningBox.setDefaultButton(QMessageBox::Abort);
    sslWarningBox.setIcon(QMessageBox::Warning);
    if (sslWarningBox.exec() == QMessageBox::Ignore)
        reply->ignoreSslErrors();
}

void LauncherWindow::loadURLListFromFile()
{
    QString selectedFile;
#ifndef QT_NO_FILEDIALOG
    selectedFile = QFileDialog::getOpenFileName(this, tr("Load URL list from file")
                                                       , QString(), tr("Text Files (*.txt);;All Files (*)"));
#endif
    if (selectedFile.isEmpty())
       return;

    m_urlLoader = new UrlLoader(this->page()->mainFrame(), selectedFile, 0, 0);
    m_urlLoader->loadNext();
}

void LauncherWindow::printURL(const QUrl& url)
{
    QTextStream output(stdout);
    output << "Loaded: " << url.toString() << endl;
}

#if !defined(QT_NO_FILEDIALOG) && !defined(QT_NO_MESSAGEBOX)
void LauncherWindow::downloadRequest(const QNetworkRequest &request)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    m_reply = manager->get(request);
    connect(m_reply, SIGNAL(finished()), this, SLOT(fileDownloadFinished()));
}

void LauncherWindow::fileDownloadFinished()
{
    QString suggestedFileName;
    if (m_reply->request().url().scheme().toLower() != QLatin1String("data")) {
        QFileInfo fileInf(m_reply->request().url().toString());
        suggestedFileName = QDir::homePath() + "/" + fileInf.fileName();
    } else
        suggestedFileName = QStringLiteral("data");
    QString fileName = QFileDialog::getSaveFileName(this, "Save as...", suggestedFileName, "All Files (*)");

    if (fileName.isEmpty())
        return;
    if (m_reply->error() != QNetworkReply::NoError)
        QMessageBox::critical(this, QStringLiteral("Download"), QStringLiteral("Download failed: ") + m_reply->errorString());
    else {
        QFile file(fileName);
        file.open(QIODevice::WriteOnly);
        file.write(m_reply->readAll());
        file.close();
        QMessageBox::information(this, QString("Download"), fileName + QString(" downloaded successfully."));
    }
}
#endif

void LauncherWindow::clearMemoryCaches()
{
    QWebSettings::clearMemoryCaches();
    qDebug() << "Memory caches were cleared";
}

void LauncherWindow::clearPageSelection()
{
    page()->triggerAction(QWebPage::Unselect);
}

void LauncherWindow::toggleLocalStorage(bool toggle)
{
    m_windowOptions.useLocalStorage = toggle;
    page()->settings()->setAttribute(QWebSettings::LocalStorageEnabled, toggle);
}

void LauncherWindow::toggleOfflineStorageDatabase(bool toggle)
{
    m_windowOptions.useOfflineStorageDatabase = toggle;
    page()->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, toggle);
}

void LauncherWindow::toggleOfflineWebApplicationCache(bool toggle)
{
    m_windowOptions.useOfflineWebApplicationCache = toggle;
    page()->settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, toggle);
}

void LauncherWindow::setOfflineStorageDefaultQuota()
{
    // For command line execution, quota size is taken from command line.   
    if (m_windowOptions.offlineStorageDefaultQuotaSize)
        page()->settings()->setOfflineStorageDefaultQuota(m_windowOptions.offlineStorageDefaultQuotaSize);
    else {
#ifndef QT_NO_INPUTDIALOG
        bool ok;
        // Maximum size is set to 25 * 1024 * 1024.
        int quotaSize = QInputDialog::getInt(this, "Offline Storage Default Quota Size" , "Quota Size", 0, 0, 26214400, 1, &ok);
        if (ok) 
            page()->settings()->setOfflineStorageDefaultQuota(quotaSize);
#endif
    }
}

void LauncherWindow::toggleScrollAnimator(bool toggle)
{
    m_windowOptions.enableScrollAnimator = toggle;
    page()->settings()->setAttribute(QWebSettings::ScrollAnimatorEnabled, toggle);
}

LauncherWindow* LauncherWindow::newWindow()
{
    LauncherWindow* mw = new LauncherWindow(&m_windowOptions);
    mw->show();
    return mw;
}

LauncherWindow* LauncherWindow::cloneWindow()
{
    LauncherWindow* mw = new LauncherWindow(&m_windowOptions, qobject_cast<QGraphicsView*>(m_view)->scene());
    mw->show();
    return mw;
}

void LauncherWindow::onKeyboardActiveChanged(bool a)
{
    if (!keyboardEnabled)
        return;
    m_keyboard->setVisible(a);
}

void LauncherWindow::onKeyboardHeightChanged(int h)
{

}

