/*
 * Copyright (C) 2009-2010 Nokia Corporation and/or its subsidiary(-ies)
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

#include "webpage.h"

#include <QApplication>
#include <QAuthenticator>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QFont>
#include <QWebEngineProfile>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkInterface>

WebPage::WebPage(QWebEngineProfile* profile, QObject* parent)
    : QWebEnginePage(profile, parent)
    , m_debugServer(nullptr)
    , m_dialogParent(nullptr)
{
    applyProxy();

    connect(this, &QWebEnginePage::authenticationRequired,
            this, &WebPage::onAuthenticationRequired);
    connect(this, &QWebEnginePage::featurePermissionRequested,
            this, &WebPage::onFeaturePermissionRequested);
}

void WebPage::applyProxy()
{
    // QtWebEngine has no per-page QNetworkAccessManager — Chromium does its own
    // networking, and honours only the application-wide proxy.
    QUrl proxyUrl(QString::fromLocal8Bit(qgetenv("http_proxy")));
    if (proxyUrl.isValid() && !proxyUrl.host().isEmpty()) {
        int proxyPort = (proxyUrl.port() > 0) ? proxyUrl.port() : 8080;
        QNetworkProxy::setApplicationProxy(
            QNetworkProxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyPort));
    }
}

bool WebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
    bool networkUp = false;
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
            && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)
            && !iface.addressEntries().isEmpty()) {
            networkUp = true;
            break;
        }
    }
    if (!networkUp && !url.matches(QUrl("http://127.0.0.1/"), QUrl::FullyDecoded)) {
        emit networkUnavailable();
        return false;
    }

    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

void WebPage::javaScriptAlert(const QUrl& securityOrigin, const QString& msg)
{
    if (m_debugServer)
        m_debugServer->sendWSMessage("ALERT:" + msg.trimmed());

    QMessageBox box(m_dialogParent);
    QFont f = box.font();
    f.setPointSize(6);
    box.setFont(f);
    box.setWindowTitle(tr("JavaScript Alert - %1").arg(securityOrigin.host()));
    box.setText(msg);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void WebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                       const QString& message,
                                       int lineNumber,
                                       const QString& sourceID)
{
    Q_UNUSED(level);
    QString ds = QString("%1:%2 : %3").arg(sourceID).arg(lineNumber).arg(message);
    if (m_debugServer)
        m_debugServer->sendWSMessage("CONSOLE: " + ds.trimmed());
}

void WebPage::onAuthenticationRequired(const QUrl& requestUrl, QAuthenticator* authenticator)
{
    if (m_locked) {
        // Nobody has signed on, so there is nobody to ask. Refusing leaves the
        // page showing its own error behind the lock, which is why the reload
        // on unlock exists.
        qWarning() << "authentication requested while locked, refused:" << requestUrl;
        m_authSuppressed = true;
        *authenticator = QAuthenticator();
        return;
    }

    QDialog* dialog = new QDialog(m_dialogParent ? m_dialogParent
                                                 : QApplication::activeWindow());
    dialog->setWindowTitle("HTTP Authentication");

    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setLayout(layout);

    QLabel* messageLabel = new QLabel(dialog);
    messageLabel->setWordWrap(true);
    messageLabel->setText(QString("Enter username and password for: %1").arg(requestUrl.toString()));
    layout->addWidget(messageLabel, 0, 1);

    QLabel* userLabel = new QLabel("Username:", dialog);
    layout->addWidget(userLabel, 1, 0);
    QLineEdit* userInput = new QLineEdit(dialog);
    layout->addWidget(userInput, 1, 1);

    QLabel* passLabel = new QLabel("Password:", dialog);
    layout->addWidget(passLabel, 2, 0);
    QLineEdit* passInput = new QLineEdit(dialog);
    passInput->setEchoMode(QLineEdit::Password);
    layout->addWidget(passInput, 2, 1);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(buttonBox, 3, 1);

    if (dialog->exec() == QDialog::Accepted) {
        authenticator->setUser(userInput->text());
        authenticator->setPassword(passInput->text());
    } else {
        // Cancelling must invalidate the authenticator, otherwise QtWebEngine
        // retries with empty credentials instead of failing the request.
        *authenticator = QAuthenticator();
    }

    delete dialog;
}

void WebPage::onFeaturePermissionRequested(const QUrl& securityOrigin, QWebEnginePage::Feature feature)
{
    setFeaturePermission(securityOrigin, feature, PermissionGrantedByUser);
}

QWebEnginePage* WebPage::createWindow(QWebEnginePage::WebWindowType)
{
    // In kiosk mode, new windows load in the same view
    return this;
}
