include(../plugin_pro.inc)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += mainwin.h main_window.h ../../include/main_window_i.h
FORMS += mainwin.ui
SOURCES += mainwin.cpp main_window.cpp
