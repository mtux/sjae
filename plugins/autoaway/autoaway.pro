include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += autoaway.h autoawayoptions.h \
	../../include/autoaway_i.h \
	../../include/options_i.h
SOURCES += autoaway.cpp autoawayoptions.cpp
FORMS += autoawayoptions.ui
