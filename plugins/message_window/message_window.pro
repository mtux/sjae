include(../plugin_pro.inc)

DEPENDPATH += . GeneratedFiles

QT += gui webkit

# Input
HEADERS += chatinput.h splitterwin.h messagewin.h messagewindow.h ../../include/message_window_i.h
FORMS += chatinput.ui splitterwin.ui messagewin.ui
SOURCES += chatinput.cpp splitterwin.cpp messagewin.cpp messagewindow.cpp
