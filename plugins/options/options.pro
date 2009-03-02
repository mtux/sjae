include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += options.h optionstree.h optionswin.h \
	../../include/options_i.h
FORMS += optionstree.ui optionswin.ui
SOURCES += options.cpp optionstree.cpp optionswin.cpp
