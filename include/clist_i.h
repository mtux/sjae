#ifndef _I_CLIST_H
#define _I_CLIST_H

#include "contact_info_i.h"
#include "menus_i.h"
#include "global_status.h"
#include <QString>
#include <QTreeWidgetItem>
#include <QAction>

#define INAME_CLIST	"CListInterface"

// tree widget item types
#define TWIT_GROUP				0x10
#define TWIT_CONTACT			0x20

#define UUID_CONTACT_CLICKED				"{7EE7F0E2-0354-427c-926F-245CACDA3AA6}"
#define UUID_CONTACT_DBL_CLICKED			"{669304BD-CC4E-46d6-BE27-E1C093C18F0E}"
#define UUID_SHOW_TIP						"{A9188B2F-919E-4a5f-9506-495E2FA0C333}"
#define UUID_HIDE_TIP						"{2C930720-D9E2-4940-B57A-247F94C046DA}"

class ContactClicked: public EventsI::Event {
public:
	ContactClicked(Contact *c, QObject *source = 0): EventsI::Event(UUID_CONTACT_CLICKED, source), contact(c) {}
	Contact *contact;
};

class ContactDblClicked: public EventsI::Event {
public:
	ContactDblClicked(Contact *c, QObject *source = 0): EventsI::Event(UUID_CONTACT_DBL_CLICKED, source), contact(c) {}
	Contact *contact;
};

class ShowTip: public EventsI::Event {
public:
	ShowTip(Contact *c, QObject *source = 0): EventsI::Event(UUID_SHOW_TIP, source), contact(c) {}
	Contact *contact;
};

class HideTip: public EventsI::Event {
public:
	HideTip(QObject *source = 0): EventsI::Event(UUID_HIDE_TIP, source) {}
};

class CListI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	const QString get_interface_name() const {return INAME_CLIST;}
};

#endif
