#ifndef _I_MENUS_H
#define _I_MENUS_H

#include "plugin_i.h"
#include "events_i.h"
#include "contact_info_i.h"

#include <QAction>
#include <QStringList>
#include <QMenu>

#define INAME_MENUS	"MenusInterface"

#define UUID_SHOW_CONTACT_MENU				"{CEC41DE5-9D8C-412a-B504-F329D7514B97}"
#define UUID_SHOW_GROUP_MENU				"{5C4A3232-E759-40d1-94A4-4B2AB8614712}"
#define UUID_SHOW_MENU						"{D6CAB1CD-86B0-439a-8A82-1AA7122FF423}"

class ShowContactMenu: public EventsI::Event {
public:
	ShowContactMenu(Contact *c, QObject *source = 0): EventsI::Event(UUID_SHOW_CONTACT_MENU, source), contact(c) {}
	Contact *contact;
};

class ShowGroupMenu: public EventsI::Event {
public:
	ShowGroupMenu(const QStringList &full_gn, int contacts, QObject *source = 0): EventsI::Event(UUID_SHOW_GROUP_MENU, source), full_group_name(full_gn), contactCount(contacts) {}
	QStringList full_group_name;
	int contactCount;
};

class ShowMenu: public EventsI::Event {
public:
	ShowMenu(const QString &id, QObject *source = 0): EventsI::Event(UUID_SHOW_MENU, source), menu_id(id) {}
	QString menu_id;
};

class MenusI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_MENUS;}

	virtual QAction *add_contact_action(const QString &label, const QString &icon = "") = 0;
	virtual QAction *add_group_action(const QString &label, const QString &icon = "") = 0;
	virtual QAction *add_menu_action(const QString &menu_id, const QString &label, const QString &icon = "", QAction *prev = 0) = 0;
	virtual QAction *add_menu_separator(const QString &menu_id, QAction *prev = 0) = 0;

	virtual void show_contact_menu(Contact *c, const QPoint &p = QPoint()) = 0;
	virtual void show_group_menu(const QStringList &full_gn, int contactCount, const QPoint &p = QPoint()) = 0;
	virtual void show_menu(const QString &id, const QPoint &p = QPoint()) = 0;

	virtual QMenu *get_menu(const QString &id) = 0;
};

#endif
