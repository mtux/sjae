include(../plugin_pro.pri)

DEPENDPATH += . \
    GeneratedFiles
QT += gui

# Input
HEADERS += smileys.h \
    smileysoptions.h \
    ../../include/options_i.h
SOURCES += smileys.cpp \
    smileysoptions.cpp
FORMS += smileysoptions.ui
