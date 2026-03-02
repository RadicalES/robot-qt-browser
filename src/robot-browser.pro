QT += core gui widgets network quickwidgets quickcontrols2 virtualkeyboard websockets webkit webkitwidgets dbus

DEFINES += QT_DEPRECATED_WARNING
DEFINES += QT_NO_DEBUG

# Version from VERSION file (single source of truth)
VERSION_FILE = $$PWD/../VERSION
APP_VERSION = $$cat($$VERSION_FILE)
DEFINES += APP_VERSION=\\\"$$APP_VERSION\\\"

CONFIG += c++14

HEADERS = \
    webpage.h \
    cookiejar.h \
    websockserver.h \
    unixsignalnotifier.h \
    networkcontroller.h \
    webpagecontroller.h \
    systemcontroller.h \
    overlayeventfilter.h

SOURCES = \
    main.cpp \
    webpage.cpp \
    cookiejar.cpp \
    websockserver.cpp \
    unixsignalnotifier.cpp \
    networkcontroller.cpp \
    webpagecontroller.cpp \
    systemcontroller.cpp

# QtWebKit 5.212
# If WEBKIT_SOURCE_DIR is set (e.g. custom Yocto build), use it.
# Otherwise fall back to system-installed libqt5webkit5-dev (Debian 12).
!isEmpty(WEBKIT_SOURCE_DIR) {
    INCLUDEPATH += \
        $$WEBKIT_SOURCE_DIR/Source/WebKit/qt/WidgetApi \
        $$WEBKIT_SOURCE_DIR/Source/WebKit/qt/Api \
        $$WEBKIT_SOURCE_DIR/Source/WebKit2/UIProcess/API/qt \
        $$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/DerivedSources/ForwardingHeaders \
        $$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/DerivedSources/ForwardingHeaders/QtWebKit \
        $$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/DerivedSources/ForwardingHeaders/QtWebKitWidgets
    LIBS += -L"$$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/lib"
}

LIBS += -lQt5WebKit -lQt5WebKitWidgets

RESOURCES += robot-browser.qrc
