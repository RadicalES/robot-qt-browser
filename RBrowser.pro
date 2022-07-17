# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

#TARGET = RBrowser
QT  += core gui widgets network opengl virtualkeyboard quickwidgets websockets

static {
    QT += svg
    QTPLUGIN += qtvirtualkeyboardplugin
}

DEFINES += QT_CORE_LIB
DEFINES += QT_DEPRECATED_WARNING
DEFINES += QT_DISABLE_DEPRECATED=0x050000
DEFINES += QT_GUI_LIB
DEFINES += QT_NETWORK_LIB
DEFINES += T_NO_CAST_TO_ASCII
DEFINES += QT_NO_DEBUG
DEFINES += QT_NO_DYNAMIC_CAST
DEFINES += QT_NO_EXCEPTIONS
DEFINES += QT_OPENGL_LIB
DEFINES += QT_PRINTSUPPORT_LIB
DEFINES += QT_USE_QSTRINGBUILDER
DEFINES += QT_WEBKITWIDGETS_LIB
DEFINES += QT_WEBKIT_LIB
DEFINES += QT_WIDGETS_LIB
DEFINES += BUILDING_WTF

# WPA
DEFINES += CONFIG_CTRL_IFACE_UNIX
DEFINES += CONFIG_CTRL_IFACE
#DEFINES += QT_NO_INPUTDIALOG

#QT_VIRTUALKEYBOARD_LAYOUT_PATH
#QT_VIRTUALKEYBOARD_STYLE

CONFIG += c++11
CONFIG += disable-desktop
CONFIG += static

HEADERS = \
   $$PWD/cookiejar.h \
   $$PWD/fpstimer.h \
   $$PWD/launcherwindow.h \
   $$PWD/locationedit.h \
   $$PWD/mainwindow.h \
   $$PWD/urlloader.h \
   $$PWD/utils.h \
   $$PWD/webinspector.h \
   $$PWD/webpage.h \
   $$PWD/webview.h \
   digitalclock.h \
   eventcontroller.h \
   keyboardwidget.h \
   websockserver.h \
   wpa_supplicant/includes.h \
   wpa_supplicant/wpa_ctrl.h \
    unixsignalnotifier.h

SOURCES = \
   $$PWD/cookiejar.cpp \
   $$PWD/fpstimer.cpp \
   $$PWD/launcherwindow.cpp \
   $$PWD/locationedit.cpp \
   $$PWD/mainwindow.cpp \
   $$PWD/qttestbrowser.cpp \
   $$PWD/urlloader.cpp \
   $$PWD/utils.cpp \
   $$PWD/webpage.cpp \
   $$PWD/webview.cpp \
   digitalclock.cpp \
   eventcontroller.cpp \
   keyboardwidget.cpp \
   websockserver.cpp \
   wpa_supplicant/os_unix.c \
   wpa_supplicant/wpa_ctrl.c \
    unixsignalnotifier.cpp

WEBKIT_SOURCE_DIR = /home/janz/data/yocto/git/webkit
WEBKIT_DIR = $$WEBKIT_SOURCE_DIR/Source/WebKit
WEBKIT2_DIR = $$WEBKIT_SOURCE_DIR/Source/WebKit2
WTF_DIR = $$WEBKIT_SOURCE_DIR/Source/WTF
FWD_HDR_DIR = $$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/DerivedSources/ForwardingHeaders

INCLUDEPATH = $$PWD/.
INCLUDEPATH += \
        $$WEBKIT_SOURCE_DIR/Source \
        $$WEBKIT_DIR/qt/WidgetApi \
        $$WEBKIT_DIR/qt/Api \
        $$WEBKIT_DIR/qt/WebCoreSupport \
        $$FWD_HDR_DIR \
        $$FWD_HDR_DIR/QtWebKit \
        $$FWD_HDR_DIR/QtWebKitWidgets \
        $$WEBKIT2_DIR/UIProcess/API/qt \
        $$WTF_DIR

#DEFINES = 

LIBS += -L"$$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/lib" -lQt5WebKit -lQt5WebKitWidgets

RESOURCES += \
    RBrowser.qrc

DISTFILES += \
    wifi.txt

