# Phase 0 spike — throwaway, not part of the robot-browser build.
#
# Builds against either Qt 5.15 (qtwebengine5-dev, via qmake) or
# Qt 6 (qt6-webengine-dev, via qmake6). Both are tested because the
# Qt 5-first vs Qt 6-direct sequencing is still an open decision.

QT += core gui widgets webenginewidgets

CONFIG += c++14
CONFIG -= app_bundle

TARGET = webengine-spike
TEMPLATE = app

SOURCES = main.cpp
