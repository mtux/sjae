include(../plugin_pro.pri)
DEPENDPATH += . \
    GeneratedFiles
QT += gui

# Input
HEADERS += filetransfer.h \
    ftprogressdialog.h
SOURCES += filetransfer.cpp \
    ftprogressdialog.cpp
HEADERS += ../../include/options_i.h \
    filetransferoptions.h
SOURCES += filetransferoptions.cpp
FORMS += filetransferoptions.ui \
    ftprogressdialog.ui
HEADERS += ../../include/filetransfer_i.h
