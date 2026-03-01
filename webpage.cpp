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
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkProxy>

WebPage::WebPage(QObject* parent)
    : QWebPage(parent)
    , m_debugServer(nullptr)
{
    applyProxy();

    connect(networkAccessManager(), SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            this, SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(this, SIGNAL(featurePermissionRequested(QWebFrame*, QWebPage::Feature)),
            this, SLOT(requestPermission(QWebFrame*, QWebPage::Feature)));
}

void WebPage::applyProxy()
{
    QUrl proxyUrl(qgetenv("http_proxy"));
    if (proxyUrl.isValid() && !proxyUrl.host().isEmpty()) {
        int proxyPort = (proxyUrl.port() > 0) ? proxyUrl.port() : 8080;
        networkAccessManager()->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyPort));
    }
}

bool WebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type)
{
    if (networkAccessManager()->networkAccessible() != QNetworkAccessManager::Accessible
        && !request.url().matches(QUrl("http://127.0.0.1/"), QUrl::FullyDecoded)) {
        QMessageBox box(view());
        QFont f = box.font();
        f.setPointSize(6);
        box.setFont(f);
        box.setWindowTitle(tr("Network Alert"));
        box.setText("Network not Online!");
        box.setStandardButtons(QMessageBox::Ok);
        box.exec();
        return false;
    }

    return QWebPage::acceptNavigationRequest(frame, request, type);
}

QString WebPage::userAgentForUrl(const QUrl& url) const
{
    if (!m_userAgent.isEmpty())
        return m_userAgent;
    return QWebPage::userAgentForUrl(url);
}

void WebPage::javaScriptAlert(QWebFrame*, const QString& msg)
{
    if (m_debugServer)
        m_debugServer->sendWSMessage("ALERT:" + msg.trimmed());

    QMessageBox box(view());
    QFont f = box.font();
    f.setPointSize(6);
    box.setFont(f);
    box.setWindowTitle(tr("JavaScript Alert - %1").arg(mainFrame()->url().host()));
    box.setText(msg);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void WebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
{
    QString ds = QString("%1:%2 : %3").arg(sourceID).arg(lineNumber).arg(message);
    if (m_debugServer)
        m_debugServer->sendWSMessage("CONSOLE: " + ds.trimmed());
}

void WebPage::authenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator)
{
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setWindowTitle("HTTP Authentication");

    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setLayout(layout);

    QLabel* messageLabel = new QLabel(dialog);
    messageLabel->setWordWrap(true);
    messageLabel->setText(QString("Enter username and password for: %1").arg(reply->url().toString()));
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
    connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));
    layout->addWidget(buttonBox, 3, 1);

    if (dialog->exec() == QDialog::Accepted) {
        authenticator->setUser(userInput->text());
        authenticator->setPassword(passInput->text());
    }

    delete dialog;
}

void WebPage::requestPermission(QWebFrame* frame, QWebPage::Feature feature)
{
    setFeaturePermission(frame, feature, PermissionGrantedByUser);
}

QWebPage* WebPage::createWindow(QWebPage::WebWindowType)
{
    // In kiosk mode, new windows load in the same view
    return this;
}
