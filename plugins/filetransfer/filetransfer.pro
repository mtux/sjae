include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += filetransfer.h
SOURCES += filetransfer.cpp
HEADERS += ../../include/options_i.h filetransferoptions.h
SOURCES += filetransferoptions.cpp
FORMS += filetransferoptions.ui
HEADERS += ../../include/filetransfer_i.h
