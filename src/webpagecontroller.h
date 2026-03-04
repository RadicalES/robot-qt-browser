#ifndef WEBPAGECONTROLLER_H
#define WEBPAGECONTROLLER_H

#include <QObject>
#include <QUrl>
#include "kioskwebview.h"
#include "webpage.h"
#include "cookiejar.h"
#include "websockserver.h"

class WebPageController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentUrl READ currentUrl NOTIFY urlChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    explicit WebPageController(QObject* parent = nullptr);

    void init(const QUrl& localUrl, const QUrl& remoteUrl, WebsockServer* debugger);
    KioskWebView* webView() { return m_webView; }

    QString currentUrl() const;
    bool loading() const { return m_loading; }

    Q_INVOKABLE void loadLocal();
    Q_INVOKABLE void loadRemote();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void goBack();

signals:
    void urlChanged();
    void loadingChanged();

private slots:
    void onUrlChanged(const QUrl& url);
    void onLoadStarted();
    void onLoadFinished(bool ok);
    void onJavaScriptWindowObjectCleared();

private:
    QString loadResourceFiles(const QStringList& paths);

    KioskWebView* m_webView;
    WebPage* m_page;
    QUrl m_localUrl;
    QUrl m_remoteUrl;
    QString m_jsPolyfills;
    QString m_cssPolyfills;
    bool m_loading;
};

#endif
