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

#ifndef mainwindow_h
#define mainwindow_h

#include "webpage.h"

#include <QMainWindow>
#include <QStringListModel>
#include <QToolBar>
#include <QProcess>
#include <QProgressDialog>
#include <QPointer>
#include <QSocketNotifier>
#include "websockserver.h"
#include "wpa_supplicant/wpa_ctrl.h"

class LocationEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(bool landscape);
    ~MainWindow();

    void addCompleterEntry(const QUrl&);

    void load(const QString& url);
    void load(const QUrl&);

    WebPage* page() const;
    void setPage(WebPage*);

    void setToolBarsVisible(bool);
    void setDebugger(WebsockServer *debugger);
    void setLandscape(bool isLandscape);

private Q_SLOTS:
    void receiveWpaMsgs();
    void refreshWifiStatus();

protected Q_SLOTS:
    void setAddressUrl(const QString&);
    void setAddressUrl(const QUrl&);
    void openFile();
    void openLocation();
    void changeLocation();
    void changeLocationHome();
    void changeLocationRemote();
    void changeLocationAbout();
    void pageBack();    
    void wifiDialog();
    void onIconChanged();
    void onLoadStarted();
    void onTitleChanged(const QString&);

protected:
    QString addressUrl() const;
    void setRemoteURL(QUrl url);
    void setLocalURL(QUrl url);
    void setImagesDir(QString dir);
    LocationEdit* m_urlEdit;
    bool m_landscape;

private:
    void buildUI(bool isLandscape);
    void setupWPA( void );
    void processWpaMsg(char* msg);
    int wpaCtrlRequest(const QString& cmd, char* buf, const size_t buflen);
    void wpaShowStatus(const char *buf);
    void wifiRestart();
    WebPage* m_page;
    QToolBar* m_toolBar;
    QStringListModel urlModel;
    QStringList urlList;    
    QUrl m_localURL;
    QUrl m_remoteURL;
    QString m_imagesdir;
    WebsockServer *m_debugger;    
    QLabel *m_wifiIcon;

    QTimer *m_wpaTimer;
    struct wpa_ctrl* m_wpaMonConn;
    QString m_wpaCtrlRequestResult;
    QPointer<QSocketNotifier>  m_wpaMsgNotifier;

};

#endif
