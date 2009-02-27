include(../plugin_pro.inc)

DEPENDPATH += . GeneratedFiles

QT += gui sql

# Input
HEADERS += history.h \
	../../include/history_i.h
SOURCES += history.cpp
