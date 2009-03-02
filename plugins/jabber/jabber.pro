include(../plugin_pro.pri)

DEPENDPATH += . GeneratedFiles

QT += network gui xml

# Input
HEADERS += jabber.h jabberctx.h newrosteritemdialog.h protooptions.h Roster.h \
	senddirect.h jabbersearchwin.h disco.h gatewayregister.h servicediscovery.h \
	asksubscribe.h gatewaylist.h \
        ../../include/core_i.h \
        ../../include/plugin_i.h \
	../../include/accounts_i.h \
        ../../include/clist_i.h \
        ../../include/options_i.h \
        ../../include/contact_info_i.h \
        ../../include/icons_i.h \
        ../../include/events_i.h \
        ../../include/main_window_i.h \
        ../../include/add_contact_i.h
FORMS += newrosteritemdialog.ui protooptions.ui senddirect.ui jabbersearchwin.ui \
	gatewayregister.ui servicediscovery.ui asksubscribe.ui gatewaylist.ui
SOURCES += jabber.cpp jabberctx.cpp newrosteritemdialog.cpp protooptions.cpp Roster.cpp \
	senddirect.cpp jabbersearchwin.cpp disco.cpp gatewayregister.cpp servicediscovery.cpp \
	asksubscribe.cpp gatewaylist.cpp

RESOURCES += jabber.qrc
