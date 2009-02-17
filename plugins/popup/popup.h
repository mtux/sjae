#ifndef __POPUPNOTIFY_H
#define __POPUPNOTIFY_H

#include <popup_i.h>
#include <options_i.h>
#include "popupoptions.h"
#include "popupwin.h"

#include <QMap>
#include <QDesktopWidget>

class Popup: public PopupI, public PopupI::PopupListener
{
	Q_OBJECT
public:
	Popup();
	~Popup();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool register_class(const PopupClass &c);

	int show_popup(const QString &className, const QString &title, const QString &text);
	void show_preview(const PopupI::PopupClass &c, const QString &title, const QString &text);

	void popup_closed(int id, PopupDoneType done);

protected slots:
	void options_applied();
	void win_closed(int id);

protected:
	CoreI *core_i;
	PopupOptions::Settings current_settings;
	PopupOptions *opt;
	QList<PopupWin *> windows;
	int nextWinId;

	void layoutPopups();
	QDesktopWidget desktop;
};

#endif // POPUPNOTIFY

