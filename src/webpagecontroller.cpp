#include "webpagecontroller.h"

#include <QWebFrame>
#include <QWebSettings>
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

    // Minimal settings for kiosk
    QWebSettings* settings = m_page->settings();
    settings->setAttribute(QWebSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebSettings::PluginsEnabled, true);

    // Suppress context menu for kiosk
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);

    connect(m_page->mainFrame(), &QWebFrame::urlChanged, this, &WebPageController::onUrlChanged);
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

void WebPageController::onLoadFinished(bool)
{
    m_loading = false;
    emit loadingChanged();
}
