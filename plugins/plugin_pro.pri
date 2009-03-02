include (../global_pro.pri)

TEMPLATE = lib
TARGET =

DEPENDPATH += . GeneratedFiles
INCLUDEPATH += ../../include GeneratedFiles .
CONFIG += plugin

MOC_DIR = GeneratedFiles
UI_DIR = GeneratedFiles

CONFIG(release, debug|release) {
        DESTDIR = ../../bin/release/plugins
}
CONFIG(debug, debug|release) {
        DESTDIR = ../../bin/debug/plugins
}
