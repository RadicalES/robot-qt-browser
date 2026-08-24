#ifndef WEBPAGECONTROLLER_H
#define WEBPAGECONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QWebEngineView>
#include "webpage.h"
#include "websockserver.h"

class QWebEngineProfile;

class WebPageController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentUrl READ currentUrl NOTIFY urlChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    explicit WebPageController(QObject* parent = nullptr);
    ~WebPageController() override;

    void init(const QUrl& localUrl, const QUrl& remoteUrl, WebsockServer* debugger);

    // Repoint the two buttons. Only used where the URLs can be edited at run
    // time, which is a developer's machine — a terminal is told once, at
    // startup, by whatever owns its configuration.
    void setUrls(const QUrl& localUrl, const QUrl& remoteUrl);
    QWebEngineView* webView() { return m_webView; }

    // The page itself, so the lock can stop it putting dialogs on screen while
    // nobody is signed on.
    WebPage* page() const { return m_page; }

    QString currentUrl() const;
    bool loading() const { return m_loading; }

    void loadLocal();
    void loadRemote();
    void reload();
    void goBack();

signals:
    void urlChanged();
    void loadingChanged();
    void loadProgress(int percent);
    void networkUnavailable();

private slots:
    void onUrlChanged(const QUrl& url);
    void onLoadStarted();
    void onLoadFinished(bool ok);

private:
    QWebEngineProfile* m_profile;
    // QPointer, not a raw pointer: the view is reparented into the main window,
    // which is destroyed before this controller, so a raw pointer would dangle
    // by the time the destructor runs.
    QPointer<QWebEngineView> m_webView;
    WebPage* m_page;
    QUrl m_localUrl;
    QUrl m_remoteUrl;
    bool m_loading;
};

#endif
