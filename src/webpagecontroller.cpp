#include "webpagecontroller.h"

#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

WebPageController::WebPageController(QObject* parent)
    : QObject(parent)
    , m_profile(nullptr)
    , m_webView(new QWebEngineView)
    , m_page(nullptr)
    , m_loading(false)
{
    // Named profile with on-disk storage. This replaces TestBrowserCookieJar:
    // Chromium persists cookies, local storage and its HTTP cache itself, so
    // the hand-rolled debounced cookie writer is no longer needed.
    //
    // Deliberately unparented: the page must be destroyed before the profile,
    // and QObject deletes children in insertion order, which would do the
    // opposite. The destructor enforces the order explicitly.
    m_profile = new QWebEngineProfile(QStringLiteral("robot-browser"));
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    const QString dataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataDir.isEmpty()) {
        QDir().mkpath(dataDir);
        m_profile->setPersistentStoragePath(dataDir + QStringLiteral("/storage"));
        m_profile->setCachePath(dataDir + QStringLiteral("/cache"));
    }

    m_page = new WebPage(m_profile, this);
    m_webView->setPage(m_page);
    m_page->setDialogParent(m_webView);

    // Kiosk engine settings. The polyfills the QtWebKit build injected —
    // fetch, Object.values/entries, URLSearchParams, CSS custom properties,
    // position:sticky, smooth scroll — are all native in Chromium, so both the
    // scripts and the two-phase injection they needed are gone.
    QWebEngineSettings* settings = m_profile->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    // Scrollbars stay on. Hiding them suited a page that fits the screen, but
    // on a long form there is then nothing to say the page continues below the
    // fold, and nothing to drag — the operator has to guess that a flick works.
    settings->setAttribute(QWebEngineSettings::ShowScrollBars, true);

    // Chromium's scrollbar is ~15px. These terminals are used in a packhouse,
    // by an operator wearing gloves, so it is widened well past what a mouse
    // would need — 28px of track with a 18px thumb inside it.
    QWebEngineScript scrollbarStyle;
    scrollbarStyle.setName("kiosk-scrollbars");
    scrollbarStyle.setInjectionPoint(QWebEngineScript::DocumentReady);
    scrollbarStyle.setWorldId(QWebEngineScript::ApplicationWorld);
    scrollbarStyle.setRunsOnSubFrames(true);
    scrollbarStyle.setSourceCode(R"JS(
(function () {
    var css = '::-webkit-scrollbar { width: 28px; height: 28px; }' +
              '::-webkit-scrollbar-track { background: #e6e6e6; }' +
              '::-webkit-scrollbar-thumb { background: #9e9e9e; border-radius: 14px;' +
              ' border: 5px solid #e6e6e6; min-height: 60px; }' +
              '::-webkit-scrollbar-thumb:active { background: #ff4500; }';
    var style = document.createElement('style');
    style.appendChild(document.createTextNode(css));
    (document.head || document.documentElement).appendChild(style);
})();
)JS");
    m_profile->scripts()->insert(scrollbarStyle);

    // Suppress context menu for kiosk
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);

    connect(m_page, &QWebEnginePage::urlChanged, this, &WebPageController::onUrlChanged);
    connect(m_page, &QWebEnginePage::loadStarted, this, &WebPageController::onLoadStarted);
    connect(m_page, &QWebEnginePage::loadFinished, this, &WebPageController::onLoadFinished);
    connect(m_page, &QWebEnginePage::loadProgress, this, &WebPageController::loadProgress);
    connect(m_page, &WebPage::networkUnavailable,
            this, &WebPageController::networkUnavailable);
}

WebPageController::~WebPageController()
{
    // QtWebEngine warns ("Release of profile requested but WebEnginePage still
    // not deleted") and can crash if a profile outlives its pages. The view may
    // already be gone here — it is owned by the main window — so detach first.
    if (m_webView)
        m_webView->setPage(nullptr);
    delete m_page;
    m_page = nullptr;
    delete m_profile;
    m_profile = nullptr;
}

void WebPageController::init(const QUrl& localUrl, const QUrl& remoteUrl, WebsockServer* debugger)
{
    m_localUrl = localUrl;
    m_remoteUrl = remoteUrl;
    m_page->setDebugger(debugger);
}

QString WebPageController::currentUrl() const
{
    return m_page->url().toString();
}

void WebPageController::setUrls(const QUrl& localUrl, const QUrl& remoteUrl)
{
    m_localUrl = localUrl;
    m_remoteUrl = remoteUrl;
}

void WebPageController::loadLocal()
{
    if (m_page->url().isValid() && m_page->url() == m_localUrl) {
        m_page->triggerAction(QWebEnginePage::Reload);
        return;
    }
    m_page->setUrl(m_localUrl);
}

void WebPageController::loadRemote()
{
    if (m_page->url().isValid() && m_page->url() == m_remoteUrl) {
        m_page->triggerAction(QWebEnginePage::Reload);
        return;
    }
    m_page->setUrl(m_remoteUrl);
}

void WebPageController::reload()
{
    m_page->triggerAction(QWebEnginePage::Reload);
}

void WebPageController::goBack()
{
    m_page->triggerAction(QWebEnginePage::Back);
}

void WebPageController::onUrlChanged(const QUrl&)
{
    emit urlChanged();
}

void WebPageController::onLoadStarted()
{
    m_loading = true;
    emit loadingChanged();
}

void WebPageController::setZoom(qreal zoom)
{
    m_zoom = zoom;
    if (m_webView)
        m_webView->setZoomFactor(zoom);
}

void WebPageController::onLoadFinished(bool ok)
{
    Q_UNUSED(ok);
    m_loading = false;
    emit loadingChanged();

    // The zoom belongs to the terminal, so it is re-applied rather than set
    // once. A page kept over a restart, an authentication page, anything that
    // replaces what is being shown - none of it should hand an operator a page
    // in a size they cannot read.
    if (m_webView && !qFuzzyCompare(m_webView->zoomFactor(), m_zoom))
        m_webView->setZoomFactor(m_zoom);

    // Keep keyboard focus on the page. A load can leave the render widget
    // without Qt focus, and then tapping an input field never reaches the
    // input method, so the on-screen keyboard does not appear.
    if (m_webView)
        m_webView->setFocus();
}
