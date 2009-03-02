include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += events.h \
	..\..\include\events_i.h
SOURCES += events.cpp
