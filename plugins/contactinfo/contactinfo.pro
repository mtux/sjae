include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += gui sql

# Input
HEADERS += contactinfo.h \
	../../include/contact_info_i.h
SOURCES += contactinfo.cpp
