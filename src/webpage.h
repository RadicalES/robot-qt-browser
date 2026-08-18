/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef webpage_h
#define webpage_h

#include <QWebEnginePage>
#include "websockserver.h"

class QAuthenticator;
class QWebEngineProfile;

class WebPage : public QWebEnginePage {
    Q_OBJECT

public:
    explicit WebPage(QWebEngineProfile* profile, QObject* parent = nullptr);

    QWebEnginePage* createWindow(QWebEnginePage::WebWindowType) override;
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) override;

    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString& message,
                                  int lineNumber,
                                  const QString& sourceID) override;
    void javaScriptAlert(const QUrl& securityOrigin, const QString& msg) override;

    void setDebugger(WebsockServer* debugger) { m_debugServer = debugger; }

    // Dialogs are parented to this widget. QtWebEngine renders into an internal
    // child widget, so the page cannot reliably find the top-level window itself.
    void setDialogParent(QWidget* parent) { m_dialogParent = parent; }

Q_SIGNALS:
    // Navigation was blocked because no network interface is up. Reported
    // rather than shown from here: a modal over the kiosk is a poor fit for a
    // packhouse dead zone, which is routine and self-correcting.
    void networkUnavailable();

private Q_SLOTS:
    void onAuthenticationRequired(const QUrl& requestUrl, QAuthenticator* authenticator);
    void onFeaturePermissionRequested(const QUrl& securityOrigin, QWebEnginePage::Feature feature);

private:
    static void applyProxy();

    WebsockServer* m_debugServer;
    QWidget* m_dialogParent;
};

#endif
