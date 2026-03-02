#include "webpagecontroller.h"

#include <QWebFrame>
#include <QWebSettings>
#include <QFile>
#include <QDebug>

static TestBrowserCookieJar* cookieJarInstance()
{
    static TestBrowserCookieJar* jar = nullptr;
    if (!jar) {
        jar = new TestBrowserCookieJar();
        jar->setDiskStorageEnabled(true);
    }
    return jar;
}

WebPageController::WebPageController(QObject* parent)
    : QObject(parent)
    , m_webView(new QWebView)
    , m_page(new WebPage(m_webView))
    , m_loading(false)
{
    m_webView->setPage(m_page);
    m_page->networkAccessManager()->setCookieJar(cookieJarInstance());
    cookieJarInstance()->setParent(nullptr); // prevent webview from deleting shared jar

    // Web engine settings for kiosk
    QWebSettings* settings = m_page->settings();
    settings->setAttribute(QWebSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebSettings::PluginsEnabled, true);
    settings->setAttribute(QWebSettings::CSSGridLayoutEnabled, true);
    settings->setAttribute(QWebSettings::CSSRegionsEnabled, true);
    settings->setAttribute(QWebSettings::ScrollAnimatorEnabled, true);
    settings->setAttribute(QWebSettings::AcceleratedCompositingEnabled, true);

    // Load polyfills from resources
    // JS polyfills: inject before page scripts (javaScriptWindowObjectCleared)
    m_jsPolyfills = loadResourceFiles({":/js/polyfills.js", ":/js/fetch.js"});
    // CSS polyfills: libraries loaded early, activated after DOM ready (loadFinished)
    m_cssPolyfills = loadResourceFiles({
        ":/js/css-vars-ponyfill.min.js",
        ":/js/stickyfill.min.js",
        ":/js/smoothscroll.min.js"
    });

    // Suppress context menu for kiosk
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);

    connect(m_page->mainFrame(), &QWebFrame::urlChanged, this, &WebPageController::onUrlChanged);
    connect(m_page->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared,
            this, &WebPageController::onJavaScriptWindowObjectCleared);
    connect(m_page, &QWebPage::loadStarted, this, &WebPageController::onLoadStarted);
    connect(m_page, &QWebPage::loadFinished, this, &WebPageController::onLoadFinished);
}

void WebPageController::init(const QUrl& localUrl, const QUrl& remoteUrl, WebsockServer* debugger)
{
    m_localUrl = localUrl;
    m_remoteUrl = remoteUrl;
    m_page->setDebugger(debugger);
}

QString WebPageController::currentUrl() const
{
    return m_page->mainFrame()->url().toString();
}

void WebPageController::loadLocal()
{
    QUrl frameUrl = m_page->mainFrame()->url();
    if (frameUrl.isValid() && frameUrl == m_localUrl) {
        m_page->triggerAction(QWebPage::Reload);
        return;
    }
    m_page->mainFrame()->load(m_localUrl);
    QWebSettings::clearMemoryCaches();
}

void WebPageController::loadRemote()
{
    QUrl frameUrl = m_page->mainFrame()->url();
    if (frameUrl.isValid() && frameUrl == m_remoteUrl) {
        m_page->triggerAction(QWebPage::Reload);
        return;
    }
    m_page->mainFrame()->load(m_remoteUrl);
    QWebSettings::clearMemoryCaches();
}

void WebPageController::reload()
{
    m_page->triggerAction(QWebPage::Reload);
}

void WebPageController::goBack()
{
    m_page->triggerAction(QWebPage::Back);
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

void WebPageController::onLoadFinished(bool ok)
{
    m_loading = false;
    emit loadingChanged();

    if (ok && !m_cssPolyfills.isEmpty()) {
        // Inject CSS polyfill libraries, then activate them
        m_page->mainFrame()->evaluateJavaScript(m_cssPolyfills);
        m_page->mainFrame()->evaluateJavaScript(QStringLiteral(
            // Activate css-vars-ponyfill (CSS Custom Properties)
            "if (typeof cssVars === 'function') {"
            "  cssVars({ watch: true, silent: true });"
            "}"
            // Activate stickyfill (position: sticky)
            "if (typeof Stickyfill !== 'undefined') {"
            "  var stickyEls = document.querySelectorAll('[style*=\"position: sticky\"], [style*=\"position:sticky\"]');"
            "  if (stickyEls.length) Stickyfill.add(stickyEls);"
            "}"
            // Activate smoothscroll (scroll-behavior: smooth)
            "if (typeof smoothscroll !== 'undefined' && typeof smoothscroll.polyfill === 'function') {"
            "  smoothscroll.polyfill();"
            "}"
        ));
    }
}

void WebPageController::onJavaScriptWindowObjectCleared()
{
    // Inject JS polyfills BEFORE any page JavaScript runs
    if (!m_jsPolyfills.isEmpty()) {
        m_page->mainFrame()->evaluateJavaScript(m_jsPolyfills);
    }
}

QString WebPageController::loadResourceFiles(const QStringList& files)
{
    QString combined;
    for (const QString& path : files) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            combined += QString::fromUtf8(f.readAll()) + "\n";
            f.close();
        } else {
            qWarning() << "WebPageController: failed to load polyfill:" << path;
        }
    }
    return combined;
}
