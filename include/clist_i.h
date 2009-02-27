#ifndef _I_CLIST_H
#define _I_CLIST_H

#include "accounts_i.h"
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
#define UUID_SHOW_CONTACT_MENU				"{CEC41DE5-9D8C-412a-B504-F329D7514B97}"
#define UUID_SHOW_GROUP_MENU				"{5C4A3232-E759-40d1-94A4-4B2AB8614712}"

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

class ShowContactMenu: public EventsI::Event {
public:
	ShowContactMenu(Contact *c, QObject *source = 0): EventsI::Event(UUID_SHOW_CONTACT_MENU, source), contact(c) {}
	Contact *contact;
};

class ShowGroupMenu: public EventsI::Event {
public:
	ShowGroupMenu(const QString &full_gn, QObject *source = 0): EventsI::Event(UUID_SHOW_GROUP_MENU, source), full_group_name(full_gn) {}
	QString full_group_name;
};


class CListI: public PluginI, public EventsI::EventListener {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	const QString get_interface_name() const {return INAME_CLIST;}

	virtual QTreeWidgetItem *add_contact(Contact *contact) = 0;
	virtual QAction *add_contact_action(Account *account, const QString &label, const QString &icon = "") = 0;

	virtual bool get_hide_offline() {return hide_offline;}

	virtual bool event_fired(EventsI::Event &e) = 0;

public slots:
	virtual void set_group_delimiter(Account *account, const QString &delim) = 0;
	virtual void remove_contact(Contact *contact) = 0;
	virtual void remove_all_contacts(Account *account) = 0;
	virtual void update_label(Contact *contact) = 0;
	virtual void update_group(Contact *contact) = 0;
	virtual void update_status(Contact *contact) = 0;

	virtual void set_hide_offline(bool hide) {hide_offline = hide;}

signals:
	//void contact_clicked(const QString &proto_name, const QString &account_id, const QString &id);
	//void contact_dbl_clicked(const QString &proto_name, const QString &account_id, const QString &id);
	//void show_tip(const QString &proto_name, const QString &account_id, const QString &id, const QPoint &p);
	//void hide_tip();
	//void aboutToShowContactMenu(const QString &proto_name, const QString &account_id, const QString &id);
	//void aboutToShowGroupMenu(const QString &full_gn);

protected:
	bool hide_offline;
};

#endif
