include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += popup.h popupoptions.h popupwin.h \
	../../include/popup_i.h ../../include/options_i.h
SOURCES += popup.cpp popupoptions.cpp popupwin.cpp
FORMS += popupoptions.ui popupwin.ui
RESOURCES += res.qrc
