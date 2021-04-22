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
#include "digitalclock.h"

MainWindow::MainWindow(bool landscape)
    : m_page(new WebPage(this))
    , m_toolBar(0)
    , m_urlEdit(0)
    , m_debugger(nullptr)
    , m_landscape(landscape)
    , m_wifiProcess(nullptr)
    , progressDialog(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose);
    if (qgetenv("QTTESTBROWSER_USE_ARGB_VISUALS").toInt() == 1)
        setAttribute(Qt::WA_TranslucentBackground);

    buildUI(landscape);
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
    QPixmap wpix(m_imagesdir + "/wifi.png");

    QAction* homeAction = m_toolBar->addAction(QIcon(hpix), "");
    QAction* remoteAction = m_toolBar->addAction(QIcon(spix), "");
    QAction* backAction = m_toolBar->addAction(QIcon(bpix), "");
    QAction* wifiAction = m_toolBar->addAction(QIcon(wpix), "");
    m_toolBar->addSeparator();
    QAction* infoAction = m_toolBar->addAction(QIcon(ipix), "");

    homeAction->setCheckable(false);
    remoteAction->setCheckable(false);
    backAction->setCheckable(false);
    infoAction->setCheckable(false);

    connect(homeAction, SIGNAL(triggered()), this, SLOT(changeLocationHome()));
    connect(remoteAction, SIGNAL(triggered()), this, SLOT(changeLocationRemote()));
    connect(infoAction, SIGNAL(triggered()), this, SLOT(changeLocationAbout()));
    connect(backAction, SIGNAL(triggered()), this, SLOT(pageBack()));
    connect(wifiAction, SIGNAL(triggered()), this, SLOT(wifiRestart()));

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar->addWidget(spacer);
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

void MainWindow::freeProcess()
{
    qDebug() << "Process done!" << endl;
    if(m_wifiProcess != NULL) {
       // qDebug() << m_wifiProcess->readAllStandardOutput();
        delete m_wifiProcess;
    }

    progressDialog->close();
    delete progressDialog;

    progressDialog = NULL;
    m_wifiProcess = NULL;

}

void MainWindow::outputProcess()
{
    if(m_wifiProcess != NULL) {
        qDebug() << m_wifiProcess->readAllStandardOutput();
    }
}

void MainWindow::wifiRestart()
{
    QString f = "/home/root/RobotBrowser/restartwifi.sh";
    progressDialog = new QProgressDialog();
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setRange(0,0);


    QLabel *lbl = new QLabel("Restarting Wifi!");
    //progressDialog->setLabelText("Restarting Wifi!");
    QFont font = lbl->font();
    //QFont f( "Arial", 10, QFont::Bold);
    font.setPointSize(8);
    font.setBold(true);
    lbl->setFont(font);
    progressDialog->setLabel(lbl);
    progressDialog->setCancelButton(0);

    QString style =
           "QProgressBar:horizontal {"
               "border: 1px solid gray;"
               "border-radius: 3px;"
                "background: white;"
                "padding: 1px;"
            "height: 20px;"
           "}";

    progressDialog->setStyleSheet(style);

    m_wifiProcess = new QProcess(this);
    connect (m_wifiProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(outputProcess()));  // connect process signals with your code
    connect( m_wifiProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(freeProcess()));
    m_wifiProcess->start(f);
    progressDialog->exec();
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
