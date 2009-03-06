#ifndef __MENUS_H
#define __MENUS_H

#include <plugin_i.h>		
#include <menus_i.h>				
#include <options_i.h> 				
#include <events_i.h>
#include <icons_i.h>
#include "menusoptions.h"

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

	QAction *add_menu_action(const QString &menu_id, const QString &label, const QString &icon = "", QAction *prev = 0);
	QAction *add_menu_separator(const QString &menu_id, QAction *prev = 0);

	void show_menu(const QString &id, const QMap<QString, QVariant> &properties, const QPoint &p = QPoint());

	QMenu *get_menu(const QString &id);

protected slots:				
	void options_applied();		
	
protected:
	CoreI *core_i;
	QPointer<EventsI> events_i;
	QPointer<IconsI> icons_i;

	MenusOptions *opt;

	QMap<QString, QMenu *> menus;
};

#endif // MENUS

