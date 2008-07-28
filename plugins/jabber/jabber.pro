include(../plugin_pro.inc)

DEPENDPATH += . GeneratedFiles

QT += gui network xml

# Input
HEADERS += jabber.h jabberctx.h newrosteritemdialog.h protooptions.h Roster.h \
  ../../include/accounts_i.h \
  ../../include/clist_i.h \
  ../../include/options_i.h
FORMS += newrosteritemdialog.ui protooptions.ui
SOURCES += jabber.cpp jabberctx.cpp newrosteritemdialog.cpp protooptions.cpp Roster.cpp
RESOURCES += jabber.qrc
