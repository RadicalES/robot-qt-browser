# robot-browser — Qt 6 + QtWebEngine build.
#
# Replaces robot-browser.pro. Debian's Qt 6 ships no aarch64 qmake mkspec, so
# cross-compiling for CM4 / Pi 5 has to go through CMake — see
# docker/toolchain-arm64.cmake and docker/build-cm4.sh.

cmake_minimum_required(VERSION 3.16)

# Version from the VERSION file (single source of truth)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" APP_VERSION)
string(STRIP "${APP_VERSION}" APP_VERSION)

project(robot-browser VERSION ${APP_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    WebSockets
    WebEngineWidgets
    QuickWidgets
    DBus
)

add_executable(robot-browser
    # Headers are listed so AUTOMOC processes them. digitalclock.h in
    # particular is a header-only Q_OBJECT class and will not link without it.
    webpage.h
    websockserver.h
    unixsignalnotifier.h
    networkcontroller.h
    webpagecontroller.h
    systemcontroller.h
    digitalclock.h
    wifidialog.h
    infodialog.h
    rebootdialog.h
    landialog.h
    ipconfigdialog.h
    ipconfig.h
    nmdefs.h
    dialogstyle.h
    pagefocusguard.h
    kioskstyle.h
    virtualkeyboardpanel.h

    main.cpp
    webpage.cpp
    websockserver.cpp
    unixsignalnotifier.cpp
    networkcontroller.cpp
    webpagecontroller.cpp
    systemcontroller.cpp
    wifidialog.cpp
    infodialog.cpp
    rebootdialog.cpp
    landialog.cpp
    ipconfigdialog.cpp
    ipconfig.cpp
    robot-browser.qrc
)

# Where the packaged QML import root lives (custom virtual keyboard style).
set(ROBOT_BROWSER_QML_DIR "/usr/share/robot-browser/qml" CACHE STRING
    "Install path of the shipped QML import root")

target_compile_definitions(robot-browser PRIVATE
    APP_VERSION="${APP_VERSION}"
    ROBOT_BROWSER_QML_DIR="${ROBOT_BROWSER_QML_DIR}"
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(robot-browser PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
    Qt6::WebEngineWidgets
    Qt6::QuickWidgets
    Qt6::DBus
)

install(TARGETS robot-browser RUNTIME DESTINATION bin)

message(STATUS "robot-browser ${APP_VERSION} — Qt ${Qt6_VERSION} + QtWebEngine")
