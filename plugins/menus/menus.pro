include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += gui

# Input
HEADERS += menus.h ../../include/options_i.h menusoptions.h ../../include/menus_i.h
SOURCES += menus.cpp menusoptions.cpp
FORMS += menusoptions.ui
