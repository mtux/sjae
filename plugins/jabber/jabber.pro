include(../plugin_pro.inc)

DEPENDPATH += . GeneratedFiles

QT += gui network xml

# Input
HEADERS += jabber.h jabberctx.h newrosteritemdialog.h protooptions.h Roster.h \
	senddirect.h jabbersearch.h disco.h gatewayregister.h servicediscovery.h \
	asksubscribe.h gatewaylist.h
  ../../include/accounts_i.h \
  ../../include/clist_i.h \
  ../../include/options_i.h
FORMS += newrosteritemdialog.ui protooptions.ui senddirect.ui jabbersearch.ui \
	gatewayregister.ui servicediscovery.ui asksubscribe.ui gatewaylist.ui
SOURCES += jabber.cpp jabberctx.cpp newrosteritemdialog.cpp protooptions.cpp Roster.cpp \
	senddirect.cpp jabbersearch.cpp disco.cpp gatewayregister.cpp servicediscovery.cpp \
	asksubscribe.cpp gatewaylist.cpp

RESOURCES += jabber.qrc
