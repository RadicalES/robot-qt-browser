QT += core gui widgets network quickwidgets quickcontrols2 virtualkeyboard websockets
# QT += dbus  # enable when implementing NetworkManager WiFi support

DEFINES += QT_DEPRECATED_WARNING
DEFINES += QT_NO_DEBUG

CONFIG += c++14

HEADERS = \
    webpage.h \
    cookiejar.h \
    websockserver.h \
    unixsignalnotifier.h \
    wpacontroller.h \
    webpagecontroller.h \
    systemcontroller.h

SOURCES = \
    main.cpp \
    webpage.cpp \
    cookiejar.cpp \
    websockserver.cpp \
    unixsignalnotifier.cpp \
    wpacontroller.cpp \
    webpagecontroller.cpp \
    systemcontroller.cpp

# QtWebKit 5.212
WEBKIT_SOURCE_DIR = /home/janz/data/yocto/git/webkit
INCLUDEPATH += \
    $$WEBKIT_SOURCE_DIR/Source/WebKit/qt/WidgetApi \
    $$WEBKIT_SOURCE_DIR/Source/WebKit/qt/Api \
    $$WEBKIT_SOURCE_DIR/Source/WebKit2/UIProcess/API/qt \
    $$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/DerivedSources/ForwardingHeaders \
    $$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/DerivedSources/ForwardingHeaders/QtWebKit \
    $$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/DerivedSources/ForwardingHeaders/QtWebKitWidgets

LIBS += -L"$$WEBKIT_SOURCE_DIR/WebKitBuild/ReleaseThud/lib" -lQt5WebKit -lQt5WebKitWidgets

RESOURCES += RBrowser.qrc
