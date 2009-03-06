#ifndef _I_MENUS_H
#define _I_MENUS_H

#include "plugin_i.h"
#include "events_i.h"

#include <QAction>
#include <QStringList>
#include <QMenu>
#include <QMap>
#include <QVariant>

#define INAME_MENUS	"MenusInterface"

#define UUID_SHOW_MENU						"{D6CAB1CD-86B0-439a-8A82-1AA7122FF423}"

class ShowMenu: public EventsI::Event {
public:
	ShowMenu(const QString &id, QObject *source = 0): EventsI::Event(UUID_SHOW_MENU, source), menu_id(id) {}
	QString menu_id;
	QMap<QString, QVariant> properties;
};

class MenusI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_MENUS;}

	virtual QAction *add_menu_action(const QString &menu_id, const QString &label, const QString &icon = "", QAction *prev = 0) = 0;
	virtual QAction *add_menu_separator(const QString &menu_id, QAction *prev = 0) = 0;

	virtual void show_menu(const QString &id, const QMap<QString, QVariant> &properties, const QPoint &p = QPoint()) = 0;

	virtual QMenu *get_menu(const QString &id) = 0;
};

#endif
