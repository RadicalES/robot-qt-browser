/*
 * Copyright (C) 2009-2010 Nokia Corporation and/or its subsidiary(-ies)
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



#include "webpage.h"

#include "launcherwindow.h"
#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QAuthenticator>
#ifndef QT_NO_DESKTOPSERVICES
#include <QDesktopServices>
#endif
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QFont>
#ifndef QT_NO_LINEEDIT
#include <QLineEdit>
#endif
#include <QMessageBox>
#include <QProgressBar>
#include <QWindow>
#include <QAbstractButton>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkProxy>
#include "QtWebSockets/qwebsocketserver.h"
#include "QtWebSockets/qwebsocket.h"

WebPage::WebPage(MainWindow* parent)
    : QWebPage(parent)
    , m_mainWindow(parent)
    , m_userAgent()
    , m_interruptingJavaScriptEnabled(false)    
    , m_debugServer(nullptr)
    , m_homedir("")
{
    applyProxy();

    connect(networkAccessManager(), SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            this, SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(this, SIGNAL(featurePermissionRequested(QWebFrame*, QWebPage::Feature)), this, SLOT(requestPermission(QWebFrame*, QWebPage::Feature)));
    connect(this, SIGNAL(featurePermissionRequestCanceled(QWebFrame*, QWebPage::Feature)), this, SLOT(featurePermissionRequestCanceled(QWebFrame*, QWebPage::Feature)));
    connect(this, &QWebPage::fullScreenRequested, this, &WebPage::requestFullScreen);
   // connect(this->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(applyScripts));
            //QObject::connect(webview->page()->mainFrame(), SIGNAL(initialLayoutCompleted()), webview, SLOT(close())));
    connect(this, SIGNAL(frameCreated(QWebFrame *)), this, SLOT(frameCreated(QWebFrame *)));
//
//    connect(mainFrame(), SIGNAL(initialLayoutCompleted()),
          //  this,  SLOT(initialLayout()));

    connect(mainFrame(), SIGNAL(javaScriptWindowObjectCleared()),
            this,  SLOT(loadJavascriptPatches()));

    //settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    //settings()->setAttribute(QWebSettings::JavascriptCanCloseWindows, true);
    //settings()->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, true);
    //QWebSettings::

    //connect(this->mainFrame(), SIGNAL(loadFinished(bool)), this, SLOT(injectJavascriptHelpers(bool)));
    //networkAccessManager()->networkAccessible()
}

void WebPage::frameCreated(QWebFrame * frame) {
  frame->connect(frame, SIGNAL(javaScriptWindowObjectCleared()),
          this,  SLOT(loadJavascriptPatches()));

//  frame->connect(frame, SIGNAL(initialLayoutCompleted()),
         // this,  SLOT(injectJavascriptHelpers()));


 // if(m_debugServer) {
   //   m_debugServer->sendWSMessage("Frame created");
  //}
}

void WebPage::initialLayout()
{
    //if(m_debugServer) {
      //  m_debugServer->sendWSMessage("initialLayout");
    //}
}

void WebPage::setAppDir(QString dirname)
{
    m_homedir = dirname;
}

void WebPage::loadJavascriptPatches() {
    QWebFrame* frame = qobject_cast<QWebFrame *>(QObject::sender());
    QDir dir(m_homedir + "/js");
    QStringList fl = dir.entryList();
    
    foreach(QString fn, fl) {
        if(fn.endsWith(".js")) {
            QString fnf(m_homedir + "/js/" + fn);
            QFile f(fnf);
            if(!f.open(QIODevice::ReadOnly)) {
                if(m_debugServer) {
                    m_debugServer->sendWSMessage("Failed to open: " + fn);
                }
            }
            else {
//                if(m_debugServer) {
//                    m_debugServer->sendWSMessage("Patching JS: " + fn);
//                }
                QString js = f.readAll();
                frame->evaluateJavaScript(js);
            }
        }
    }
}

void WebPage::setDebugger(WebsockServer *debugger)
{
    m_debugServer = debugger;
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
    QObject* viewer = parent();

    if((networkAccessManager()->networkAccessible() != QNetworkAccessManager::Accessible) && (!request.url().matches(QUrl("http://127.0.0.1/"), QUrl::FullyDecoded))) {
        QMessageBox box(view());
        QFont f = box.font();
        f.setPointSize(6);
        box.setFont(f);
        box.setWindowTitle(tr("Network Alert - %1").arg(mainFrame()->url().host()));
        box.setTextFormat(Qt::PlainText);
        box.setText("Network not Online! ");
        box.setStandardButtons(QMessageBox::Ok);
        box.exec();

        return false;
    }

    QVariant value = viewer->property("keyboardModifiers");

    if (!value.isNull()) {
        Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(value.toInt());

        if (modifiers & Qt::ShiftModifier) {
            QWebPage* page = createWindow(QWebPage::WebBrowserWindow);
            page->mainFrame()->load(request);
            return false;
        }

        if (modifiers & Qt::AltModifier) {
            openUrlInDefaultBrowser(request.url());
            return false;
        }
    }

    return QWebPage::acceptNavigationRequest(frame, request, type);
}

void WebPage::openUrlInDefaultBrowser(const QUrl& url)
{
#ifndef QT_NO_DESKTOPSERVICES
    if (QAction* action = qobject_cast<QAction*>(sender()))
        QDesktopServices::openUrl(action->data().toUrl());
    else
        QDesktopServices::openUrl(url);
#endif
}

QString WebPage::userAgentForUrl(const QUrl& url) const
{
    if (!m_userAgent.isEmpty())
        return m_userAgent;
    return QWebPage::userAgentForUrl(url);
}

bool WebPage::shouldInterruptJavaScript()
{
    if (!m_interruptingJavaScriptEnabled)
        return false;

    return QWebPage::shouldInterruptJavaScript();
}

void WebPage::javaScriptAlert(QWebFrame * frame, const QString & msg)
{
    QString s = "ALERT:" + msg.trimmed();
   // qDebug() << s;
    if(m_debugServer) {
        m_debugServer->sendWSMessage(s);
    }

    QMessageBox box(view());
    QFont f = box.font();
    f.setPointSize(6);
    box.setFont(f);
    box.setWindowTitle(tr("JavaScript Alert - %1").arg(mainFrame()->url().host()));
    box.setTextFormat(Qt::PlainText);
    box.setText(msg);
    box.setStandardButtons(QMessageBox::Ok);

    box.exec();

    //QWebPage::javaScriptAlert(frame, msg);
}

void WebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
{
    QString ln = QString("%1").arg(lineNumber, 6, 'g', -1, '0');
    QString ds = QString("%1:%2").arg(sourceID, ln) + " : " + message;
    QString s = "CONSOLE: " + ds.trimmed();
    //qDebug() << s;
    if(m_debugServer) {
        m_debugServer->sendWSMessage(s);
    }
}

void WebPage::authenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator)
{
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setWindowTitle("HTTP Authentication");

    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setLayout(layout);

    QLabel* messageLabel = new QLabel(dialog);
    messageLabel->setWordWrap(true);
    QString messageStr = QString("Enter with username and password for: %1");
    messageLabel->setText(messageStr.arg(reply->url().toString()));
    layout->addWidget(messageLabel, 0, 1);

#ifndef QT_NO_LINEEDIT
    QLabel* userLabel = new QLabel("Username:", dialog);
    layout->addWidget(userLabel, 1, 0);
    QLineEdit* userInput = new QLineEdit(dialog);
    layout->addWidget(userInput, 1, 1);

    QLabel* passLabel = new QLabel("Password:", dialog);
    layout->addWidget(passLabel, 2, 0);
    QLineEdit* passInput = new QLineEdit(dialog);
    passInput->setEchoMode(QLineEdit::Password);
    layout->addWidget(passInput, 2, 1);
#endif

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
            | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
    connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));
    layout->addWidget(buttonBox, 3, 1);

    if (dialog->exec() == QDialog::Accepted) {
#ifndef QT_NO_LINEEDIT
        authenticator->setUser(userInput->text());
        authenticator->setPassword(passInput->text());
#endif
    }

    delete dialog;
}

void WebPage::infoDialog()
{
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setWindowTitle("About");

    dialog->setObjectName("dialog");
    dialog->setStyleSheet("#dialog{"
                          "border:2px solid black;"
                          "background-color: grey;"
                          "font-size: 6px;"
                          "}");


    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setLayout(layout);

    QLabel* messageLabel = new QLabel(dialog);
    //messageLabel->setWordWrap(true);
    QString messageStr = QString("RobotBrowser Version 1.5");
    messageLabel->setText(messageStr);
    QFont font = messageLabel->font();
    font.setPointSize(6);
    messageLabel->setFont(font);
    layout->addWidget(messageLabel, 0, 1);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Reset, Qt::Horizontal, dialog);
    QPushButton* restoreButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);
    QPushButton* resetButton = buttonBox->button(QDialogButtonBox::Reset);
    resetButton->setText("Reboot");

    buttonBox->setObjectName("button");

    QList<QWidget*> wl = buttonBox->findChildren<QWidget*>();
    foreach(QWidget *w, wl) {
        QFont f = w->font();
        f.setPointSize(6);
        w->setFont(f);
    }

    connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));

    connect(buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(resetDefaultsPressed(QAbstractButton *)));

    layout->addWidget(buttonBox, 3, 1);

    int res = dialog->exec();

    qDebug("Result: = %d", res);

    if (res == QDialog::Accepted) {
    }

    delete dialog;
}

void WebPage::resetDefaultsPressed(QAbstractButton *button)
{
    if (button->text() == tr("Reboot")) {
        rebootDialog();
    }
    else if (button->text() != tr("OK")) {
        resetDialog();
    }
}

void WebPage::resetDialog()
{
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setWindowTitle("Reset Settings");

    dialog->setObjectName("dialog");
    dialog->setStyleSheet("#dialog{"
                          "border:2px solid black;"
                          "background-color: red;"
                          "font-size: 6px;"
                          "}");


    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setLayout(layout);

    QLabel* messageLabel = new QLabel(dialog);
    //messageLabel->setWordWrap(true);
    QString messageStr = QString("OK to reset to factory defaults?\nYour device will restart once completed.");
    messageLabel->setText(messageStr);
    QFont font = messageLabel->font();
    font.setPointSize(6);
    messageLabel->setFont(font);
    layout->addWidget(messageLabel, 0, 1);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
  //  QPushButton* resetButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);


    buttonBox->setObjectName("button");

    QList<QWidget*> wl = buttonBox->findChildren<QWidget*>();
    foreach(QWidget *w, wl) {
        QFont f = w->font();
        f.setPointSize(6);
        w->setFont(f);
    }

    connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    layout->addWidget(buttonBox, 3, 1);

    if(dialog->exec() == QDialog::Accepted) {
        //QString file = "/file.exe";
        QString f = "/home/root/RobotBrowser/resetDefaults.sh";
        //QProcess::execute("//home//root//RobotBrowser//resetDefaults.sh");
        //QProcess *process = new QProcess(this);
       // process->start(f);
        //process.startDetached("/bin/sh", QStringList()<< "/home/pi/update.sh");
        //setCursorVisible(true);

        QProcess::execute("/bin/sh", QStringList()<< f);

        //QApplication::setCursorVisible(false);
    }

    delete dialog;
}

void WebPage::rebootDialog()
{
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setWindowTitle("Reboot Device");

    dialog->setObjectName("dialog");
    dialog->setStyleSheet("#dialog{"
                          "border:2px solid black;"
                          "background-color: red;"
                          "font-size: 6px;"
                          "}");


    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setLayout(layout);

    QLabel* messageLabel = new QLabel(dialog);
    //messageLabel->setWordWrap(true);
    QString messageStr = QString("OK to restart?\nYour device will restart safely.");
    messageLabel->setText(messageStr);
    QFont font = messageLabel->font();
    font.setPointSize(6);
    messageLabel->setFont(font);
    layout->addWidget(messageLabel, 0, 1);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
  //  QPushButton* resetButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);


    buttonBox->setObjectName("button");

    QList<QWidget*> wl = buttonBox->findChildren<QWidget*>();
    foreach(QWidget *w, wl) {
        QFont f = w->font();
        f.setPointSize(6);
        w->setFont(f);
    }

    connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    layout->addWidget(buttonBox, 3, 1);

    if(dialog->exec() == QDialog::Accepted) {
        QProcess::execute("/sbin/reboot");
    }

    delete dialog;
}

void WebPage::requestPermission(QWebFrame* frame, QWebPage::Feature feature)
{
    setFeaturePermission(frame, feature, PermissionGrantedByUser);
}

void WebPage::featurePermissionRequestCanceled(QWebFrame*, QWebPage::Feature)
{
}

void WebPage::requestFullScreen(QWebFullScreenRequest request)
{
    if (request.toggleOn()) {
        if (QMessageBox::question(view(), "Enter Full Screen Mode", "Do you want to enter full screen mode?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
            request.accept();
            m_mainWindow->setToolBarsVisible(false);
            m_mainWindow->showFullScreen();
            return;
        }
    } else {
        request.accept();
        m_mainWindow->setToolBarsVisible(true);
        m_mainWindow->showNormal();
        return;
    }
    request.reject();
}
QWebPage* WebPage::createWindow(QWebPage::WebWindowType type)
{
    LauncherWindow* mw = new LauncherWindow;
    if (type == WebModalDialog)
        mw->setWindowModality(Qt::ApplicationModal);
    mw->show();
    return mw->page();
}

QObject* WebPage::createPlugin(const QString &classId, const QUrl&, const QStringList&, const QStringList&)
{
    if (classId == "alien_QLabel") {
        QLabel* l = new QLabel;
        l->winId();
        return l;
    }

    if (classId == QLatin1String("QProgressBar"))
        return new QProgressBar(view());
    if (classId == QLatin1String("QLabel"))
        return new QLabel(view());
    return 0;
}



