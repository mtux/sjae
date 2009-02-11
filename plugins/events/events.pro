include(../plugin_pro.inc)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += events.h \
	..\..\include\events_i.h
SOURCES += events.cpp
