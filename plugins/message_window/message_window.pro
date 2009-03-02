include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += gui webkit

# Input
HEADERS += chatinput.h splitterwin.h messagewin.h messagewindow.h messagewindowoptions.h \
	../../include/message_window_i.h ../../include/options_i.h
FORMS += chatinput.ui splitterwin.ui messagewin.ui messagewindowoptions.ui
SOURCES += chatinput.cpp splitterwin.cpp messagewin.cpp messagewindow.cpp messagewindowoptions.cpp
