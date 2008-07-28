include(../plugin_pro.inc)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += messagewin.h messagewindow.h ../../include/message_window_i.h
FORMS += messagewin.ui
SOURCES += messagewin.cpp messagewindow.cpp
