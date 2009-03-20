include(../plugin_pro.pri)
DEPENDPATH += . \
    GeneratedFiles
QT += gui

# Input
HEADERS += filetransfer.h \
    ftprogressdialog.h \
    ftid.h \
    ftacceptdialog.h
SOURCES += filetransfer.cpp \
    ftprogressdialog.cpp \
    ftid.cpp \
    ftacceptdialog.cpp
HEADERS += ../../include/options_i.h \
    filetransferoptions.h
SOURCES += filetransferoptions.cpp
FORMS += filetransferoptions.ui \
    ftprogressdialog.ui \
    ftacceptdialog.ui
HEADERS += ../../include/filetransfer_i.h
