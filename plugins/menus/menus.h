#ifndef __MENUS_H
#define __MENUS_H

#include <plugin_i.h>		
#include <menus_i.h>				
#include <options_i.h> 				
#include <events_i.h>
#include <icons_i.h>
#include "menusoptions.h"

#include <QMenu>

class Menus: public MenusI 	
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	Menus();
	~Menus();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	QAction *add_contact_action(const QString &label, const QString &icon = "");
	QAction *add_group_action(const QString &label, const QString &icon = "");
	QAction *add_menu_action(const QString &menu_id, const QString &label, const QString &icon = "");

	void show_contact_menu(Contact *c, const QPoint &p = QPoint());
	void show_group_menu(const QStringList &full_gn, int contactCount, const QPoint &p = QPoint());
	void show_menu(const QString &id, const QPoint &p = QPoint());

protected slots:				
	void options_applied();		
	
protected:
	CoreI *core_i;
	QPointer<EventsI> events_i;
	QPointer<IconsI> icons_i;

	MenusOptions *opt;

	QMenu *contact_menu, *group_menu;
	QMap<QString, QMenu *> menus;
};

#endif // MENUS

