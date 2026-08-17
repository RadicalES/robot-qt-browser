# Phase 0 spike — CMake build.
#
# Debian's Qt 6 ships no aarch64 qmake mkspec, so cross-compiling for CM4/Pi 5
# has to go through CMake. The .pro file is kept for convenience when building
# natively on the device; this file is what the cross-compile container uses.

cmake_minimum_required(VERSION 3.16)
project(webengine-spike LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets WebEngineWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets WebEngineWidgets)

add_executable(webengine-spike main.cpp)

target_link_libraries(webengine-spike PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
)

message(STATUS "Building spike against Qt ${QT_VERSION_MAJOR} (${QT_VERSION})")
