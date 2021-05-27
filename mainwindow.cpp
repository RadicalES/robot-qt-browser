/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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

#include <cstdlib>

#include "mainwindow.h"

#include "locationedit.h"
#include "utils.h"

#include <QAction>
#ifndef QT_NO_INPUTDIALOG
#include <QCompleter>
#endif
#ifndef QT_NO_FILEDIALOG
#include <QFileDialog>
#endif
#include <QMenuBar>
#include <QMessageBox>
#include <QDebug>
#include <QStatusBar>
//#include <QWidgetAction>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QDialogButtonBox>
#include "digitalclock.h"


MainWindow::MainWindow(bool landscape)
    : m_page(new WebPage(this))
    , m_toolBar(0)
    , m_urlEdit(0)
    , m_debugger(nullptr)
    , m_landscape(landscape)
    , m_wpaTimer(nullptr)
    , m_wpaMonConn(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose);
    if (qgetenv("QTTESTBROWSER_USE_ARGB_VISUALS").toInt() == 1)
        setAttribute(Qt::WA_TranslucentBackground);

    buildUI(landscape);

    /* Wifi timer */
    m_wpaTimer = new QTimer();
    m_wpaTimer->setInterval(1000);
    connect(m_wpaTimer,&QTimer::timeout,this,&MainWindow::refreshWifiStatus);
    m_wpaTimer->start();

}

MainWindow::~MainWindow()
{
    delete m_wpaMsgNotifier;

    if (m_wpaMonConn) {
        wpa_ctrl_detach(m_wpaMonConn);
        wpa_ctrl_close(m_wpaMonConn);
    }

    if(m_wpaTimer) {
        m_wpaTimer->stop();
    }
}

void MainWindow::buildUI(bool isLandscape)
{

    delete m_toolBar;

    m_toolBar = new QToolBar("Navigation", this);
    m_toolBar->setFixedHeight(40);

    m_toolBar->setIconSize(QSize(38, 38));

    QPixmap bpix(m_imagesdir + "/back.png");
    QPixmap hpix(m_imagesdir + "/home.png");
    QPixmap spix(m_imagesdir + "/store.png");
    QPixmap ipix(m_imagesdir + "/info.png");
    QPixmap wpix(m_imagesdir + "/wifi-settings.png");
    QPixmap dpix(m_imagesdir + "/wifi-off.png");

    QAction* homeAction = m_toolBar->addAction(QIcon(hpix), "");
    QAction* remoteAction = m_toolBar->addAction(QIcon(spix), "");
    QAction* backAction = m_toolBar->addAction(QIcon(bpix), "");
    m_toolBar->addSeparator();
    QAction* wifiAction = m_toolBar->addAction(QIcon(wpix), "");
    QAction* infoAction = m_toolBar->addAction(QIcon(ipix), "");

    homeAction->setCheckable(false);
    remoteAction->setCheckable(false);
    backAction->setCheckable(false);
    infoAction->setCheckable(false);

    connect(homeAction, SIGNAL(triggered()), this, SLOT(changeLocationHome()));
    connect(remoteAction, SIGNAL(triggered()), this, SLOT(changeLocationRemote()));
    connect(infoAction, SIGNAL(triggered()), this, SLOT(changeLocationAbout()));
    connect(backAction, SIGNAL(triggered()), this, SLOT(pageBack()));
    connect(wifiAction, SIGNAL(triggered()), this, SLOT(wifiDialog()));

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar->addWidget(spacer);

    m_wifiIcon = new QLabel(this);
    m_wifiIcon->setPixmap(dpix);
    m_toolBar->addWidget(m_wifiIcon);
    m_toolBar->addWidget(new DigitalClock(m_toolBar));

 #ifndef QT_NO_INPUTDIALOG
    if(!isLandscape) {

        // figure out out view type
        // landscape only has toolbar at the bottom
        // portrait has toolbar top and status bar at the bottom

        if(!m_urlEdit) {
            statusBar()->setMaximumHeight(30);
            m_urlEdit = new LocationEdit(statusBar());
            m_urlEdit->setSizePolicy(QSizePolicy::Expanding, m_urlEdit->sizePolicy().verticalPolicy());
            //m_urlEdit->setFixedHeight(30);
            connect(m_urlEdit, SIGNAL(returnPressed()), SLOT(changeLocation()));
            QCompleter* completer = new QCompleter(statusBar());
            m_urlEdit->setCompleter(completer);
            completer->setModel(&urlModel);
            statusBar()->addWidget(m_urlEdit, 1);
            statusBar()->setStyleSheet("font-size: 10px");
        }

        addToolBar(Qt::TopToolBarArea, m_toolBar);
    }
    else {
       // if(m_urlEdit) {
           // delete m_urlEdit;
      //  }

        m_urlEdit = new LocationEdit(m_toolBar);
        m_urlEdit->setSizePolicy(QSizePolicy::Expanding, m_urlEdit->sizePolicy().verticalPolicy());
        connect(m_urlEdit, SIGNAL(returnPressed()), SLOT(changeLocation()));
        QCompleter* completer = new QCompleter(m_toolBar);
        m_urlEdit->setCompleter(completer);
        completer->setModel(&urlModel);
        m_toolBar->addSeparator();
        m_toolBar->addWidget(m_urlEdit);
        addToolBar(Qt::BottomToolBarArea, m_toolBar);
    }

    connect(page()->mainFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(setAddressUrl(QUrl)));
    connect(page(), SIGNAL(loadProgress(int)), m_urlEdit, SLOT(setProgress(int)));
#endif

    connect(page()->mainFrame(), SIGNAL(loadStarted()), this, SLOT(onLoadStarted()));
    connect(page()->mainFrame(), SIGNAL(iconChanged()), this, SLOT(onIconChanged()));
    connect(page()->mainFrame(), SIGNAL(titleChanged(QString)), this, SLOT(onTitleChanged(QString)));
    connect(page(), SIGNAL(windowCloseRequested()), this, SLOT(close()));
    //connect(page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared), this, SLOT(loadJavaScript()));

}

void MainWindow::refreshWifiStatus()
{
    size_t len(2048);
    char buf[len];

    if(m_wpaMonConn == NULL) {
        setupWPA();
    }
    else {
        if(wpaCtrlRequest("SIGNAL_POLL", buf, len) >= 0) {
            wpaShowStatus(buf);
        }
        else {
            QPixmap *wpix = new QPixmap(m_imagesdir + "/wifi-off.png");
            m_wifiIcon->setPixmap(*wpix);
            delete m_wpaMsgNotifier;
            wpa_ctrl_detach(m_wpaMonConn);
            wpa_ctrl_close(m_wpaMonConn);
            m_wpaMonConn = NULL;
        }
    }
}

void MainWindow::processWpaMsg(char* msg)
{
    qDebug() << msg;
}

void MainWindow::receiveWpaMsgs()
{
    char buf[256];
    size_t len;

    qDebug() << "receiveMsgs() >>>";

    while (m_wpaMonConn && wpa_ctrl_pending(m_wpaMonConn) > 0) {
        len = sizeof(buf) - 1;
        if (wpa_ctrl_recv(m_wpaMonConn, buf, &len) == 0) {
            buf[len] = '\0';
            processWpaMsg(buf);
        }
    }

    //debug("receiveMsgs() >>>>>>");
    //updateStatus(tally.contains(StatusNeedsUpdate) || tally.contains(NetworkNeedsUpdate));
    //debug("receiveMsgs() <<<<<<");
}

void MainWindow::wpaShowStatus(const char *buf)
{
    QHash<QString, QString> status;
    QStringList lines = QString(buf).split('\n');
   // qDebug() << "wpaShowStatus" << (buf);

    foreach(QString line, lines) {
        QString key = line.section('=', 0, 0);
        QString val = line.section('=', 1, 1);
        status.insert(key, val);
       // qDebug() << line;
    }

    /*
        "RSSI=-46"
        "LINKSPEED=0"
        "NOISE=-256"
        "FREQUENCY=0"
     */

    int rssi = atoi(status.value("RSSI").toUtf8());
    QPixmap *wpix;

    if (rssi >= -60)
        wpix = new QPixmap(m_imagesdir + "/wifi-4.png");
    else if (rssi >= -68)
        wpix = new QPixmap(m_imagesdir + "/wifi-3.png");
    else if (rssi >= -76)
        wpix = new QPixmap(m_imagesdir + "/wifi-2.png");
    else if (rssi >= -84)
        wpix = new QPixmap(m_imagesdir + "/wifi-1.png");
    else
        wpix = new QPixmap(m_imagesdir + "/wifi-0.png");

    m_wifiIcon->setPixmap(*wpix);
    delete wpix;
}

int MainWindow::wpaCtrlRequest(const QString& cmd, char* buf, const size_t buflen)
{
    size_t len = buflen;
    int stat;

    if (m_wpaMonConn == NULL) {
        stat = -3;
        return stat;
    }

    stat = wpa_ctrl_request(m_wpaMonConn
                                                , cmd.toLocal8Bit().constData()
                                                , strlen(cmd.toLocal8Bit().constData())
                                                , buf, &len, NULL);

    if (stat == -2)
        qDebug("'%s' command timed out.", cmd.toLocal8Bit().constData());
    else if (stat < 0)
        qDebug("'%s' command failed.", cmd.toLocal8Bit().constData());

    buf[len] = '\0';
    m_wpaCtrlRequestResult = QString(buf);
    m_wpaCtrlRequestResult.remove(QRegExp("^\""));
    m_wpaCtrlRequestResult.remove(QRegExp("\"$"));

    if (m_wpaCtrlRequestResult.startsWith("FAIL\n")) {
        m_wpaCtrlRequestResult.clear();
        stat = -1;
    }
    else {
      //  qDebug() << m_wpaCtrlRequestResult;
    }

    return stat;
}

void MainWindow::setupWPA()
{
    m_wpaMonConn = wpa_ctrl_open("/var/run/wpa_supplicant/wlan0");

    if(m_wpaMonConn != NULL) {
        if(wpa_ctrl_attach(m_wpaMonConn)) {
            qDebug() << "Failed to attached WPA";
            wpa_ctrl_close(m_wpaMonConn);
            m_wpaMonConn = NULL;
        }
        else {
            qDebug() << "WPA OK!";
            m_wpaMsgNotifier = new QSocketNotifier(wpa_ctrl_get_fd(m_wpaMonConn), QSocketNotifier::Read, this);
            connect(m_wpaMsgNotifier, SIGNAL(activated(int)), SLOT(receiveWpaMsgs()));
            size_t len(2048); char buf[len];
            //if (wpaCtrlRequest("STATUS", buf, len) < 0) {
            if(wpaCtrlRequest("SIGNAL_POLL", buf, len) < 0) {;
                qDebug() << "WPA STATUS FAILED!";
            }
            else {
                wpaShowStatus(buf);
            }

        }
    }
    else {
        qDebug() << "Failed to setup WPA";
    }

}

void MainWindow::setPage(WebPage* page)
{
    if (page && m_page)
        page->setUserAgent(m_page->userAgentForUrl(QUrl()));

    delete m_page;
    m_page = page;
    m_page->setDebugger(m_debugger);

   // qDebug("Set PAGE->>>");

    buildUI(m_landscape);
   // menuBar()->setVisible(false);
}

void MainWindow::setDebugger(WebsockServer *debugger)
{
    m_debugger = debugger;
    m_page->setDebugger(m_debugger);
}

void MainWindow::setLandscape(bool isLandscape)
{
    m_landscape = isLandscape;
}

void MainWindow::setToolBarsVisible(bool visible)
{
    //if (menuBar())
      //  menuBar()->setVisible(visible);

    if (m_toolBar)
        m_toolBar->setVisible(visible);

    if(m_landscape) {
        if(statusBar())
            statusBar()->setVisible(visible);
    }
}

WebPage* MainWindow::page() const
{
    return m_page;
}

void MainWindow::setAddressUrl(const QUrl& url)
{
    setAddressUrl(url.toString(QUrl::RemoveUserInfo));
}

void MainWindow::setAddressUrl(const QString& url)
{
#ifndef QT_NO_INPUTDIALOG
    if (!url.contains("about:"))
        m_urlEdit->setText(url);
#endif
}

void MainWindow::addCompleterEntry(const QUrl& url)
{
    QUrl::FormattingOptions opts;
    opts |= QUrl::RemoveScheme;
    opts |= QUrl::RemoveUserInfo;
    opts |= QUrl::StripTrailingSlash;
    QString s = url.toString(opts);
    s = s.mid(2);
    if (s.isEmpty())
        return;

    if (!urlList.contains(s))
        urlList += s;
    urlModel.setStringList(urlList);
}

void MainWindow::setRemoteURL(QUrl url)
{
    m_remoteURL = url;
}

void MainWindow::setLocalURL(QUrl url)
{
    m_localURL = url;
}

void MainWindow::setImagesDir(QString dir)
{
    m_imagesdir = dir;
}

void MainWindow::load(const QString& url)
{
    QUrl qurl = urlFromUserInput(url);
    if (qurl.scheme().isEmpty())
        qurl = QUrl("http://" + url + "/");
    load(qurl);
}

void MainWindow::load(const QUrl& url)
{
    if (!url.isValid())
        return;

    setAddressUrl(url.toString());
    page()->setAppDir(m_imagesdir);
    page()->mainFrame()->load(url);
    QWebSettings::clearMemoryCaches();
    //webView->history()->clear();
}

QString MainWindow::addressUrl() const
{
#ifndef QT_NO_INPUTDIALOG
    return m_urlEdit->text();
#endif
    return QString();
}

void MainWindow::changeLocation()
{
#ifndef QT_NO_INPUTDIALOG
    QString string = m_urlEdit->text();
    QUrl mainFrameURL = page()->mainFrame()->url();

    if (mainFrameURL.isValid() && string == mainFrameURL.toString()) {
        page()->triggerAction(QWebPage::Reload);
        return;
    }

    load(string);
#endif
}

void MainWindow::changeLocationHome()
{
#ifndef QT_NO_INPUTDIALOG
    QString string = m_localURL.toString();
    QUrl mainFrameURL = page()->mainFrame()->url();

    if (mainFrameURL.isValid() && string == mainFrameURL.toString()) {
        page()->triggerAction(QWebPage::Reload);
        return;
    }

    load(string);
#endif
}

void MainWindow::changeLocationRemote()
{
#ifndef QT_NO_INPUTDIALOG
    QString string = m_remoteURL.toString();
    QUrl mainFrameURL = page()->mainFrame()->url();

    if (mainFrameURL.isValid() && string == mainFrameURL.toString()) {
        page()->triggerAction(QWebPage::Reload);
        return;
    }

    load(string);
#endif
}

void MainWindow::changeLocationAbout()
{
    page()->infoDialog();
}

void MainWindow::pageBack()
{
    page()->triggerAction(QWebPage::Back);
}


void MainWindow::wifiDialog()
{
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setWindowTitle("Restart Wifi");

    dialog->setObjectName("dialog");
    dialog->setStyleSheet("#dialog{"
                          "border:2px solid black;"
                          "background-color: yellow;"
                          "font-size: 6px;"
                          "}");


    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setLayout(layout);

    QLabel* messageLabel = new QLabel(dialog);
    //messageLabel->setWordWrap(true);
    QString messageStr = QString("OK to restart Wifi Connection?");
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
        QProcess::execute("/usr/sbin/wpa_cli", QStringList() << "-iwlan0" << "reconf");
    }

    delete dialog;
}


void MainWindow::wifiRestart()
{
   char cmdbuf[256];
   char cmdresult[256]; // set an appropriate length to store each line of output
   bzero(cmdbuf, 256);
   bzero(cmdresult, 256);
   sprintf(cmdbuf, "wpa_cli -iwlan0 reconf");
   FILE *pp = popen(cmdbuf, "r"); // Create Pipeline
   fgets(cmdresult, sizeof(cmdresult), pp); //""
   pclose(pp);

   if(strstr(cmdresult, "FAIL"))
       qDebug() << "Wifi restart failed!";
   else
       qDebug() << "Wifi restart OK!";
}

void MainWindow::openFile()
{
#ifndef QT_NO_FILEDIALOG
    static const QString filter("HTML Files (*.htm *.html);;Text Files (*.txt);;Image Files (*.gif *.jpg *.png);;All Files (*)");

    QFileDialog fileDialog(this, tr("Open"), QString(), filter);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setOptions(QFileDialog::ReadOnly);

    if (fileDialog.exec()) {
        QString selectedFile = fileDialog.selectedFiles()[0];
        if (!selectedFile.isEmpty())
            load(QUrl::fromLocalFile(selectedFile));
    }
#endif
}

void MainWindow::openLocation()
{
#ifndef QT_NO_INPUTDIALOG
    m_urlEdit->selectAll();
    m_urlEdit->setFocus();
#endif
}

void MainWindow::onIconChanged()
{
#ifndef QT_NO_INPUTDIALOG
    m_urlEdit->setPageIcon(page()->mainFrame()->icon());
#endif
}

void MainWindow::onLoadStarted()
{
#ifndef QT_NO_INPUTDIALOG
    m_urlEdit->setPageIcon(QIcon());
#endif
}

void MainWindow::onTitleChanged(const QString& title)
{
    if (title.isEmpty())
        setWindowTitle(QCoreApplication::applicationName());
    else
        setWindowTitle(QString::fromLatin1("%1 - %2").arg(title).arg(QCoreApplication::applicationName()));
}
