include (../global_pro.inc)

TEMPLATE = lib
TARGET =

DEPENDPATH += .
INCLUDEPATH += ../../include
CONFIG += plugin

CONFIG(release, debug|release) {
        DESTDIR = ../../bin/release/plugins
}
CONFIG(debug, debug|release) {
        DESTDIR = ../../bin/debug/plugins
}
