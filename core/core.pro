include (../global_pro.pri)

TEMPLATE = app
TARGET = core 
DEPENDPATH += .
INCLUDEPATH += ../include

# Input
SOURCES += main.cpp core.cpp arc4.cpp
HEADERS += core.h arc4.h \
	..\include\core_i.h

CONFIG(release, debug|release) {
        DESTDIR = ../bin/release
}
CONFIG(debug, debug|release) {
        DESTDIR = ../bin/debug
}
