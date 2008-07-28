#ifndef OPTIONS_H
#define OPTIONS_H

#include <options_i.h>
#include <main_window_i.h>
#include <QPointer>
#include "optionswin.h"

class options: public OptionsI {
public:
	options();
	~options();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool add_page(const QString &category, OptionsPageI *page);
public slots:
	void show_options();

protected:
	CoreI *core_i;
	QPointer<MainWindowI> main_win_i;

	OptionsWin *win;
};

#endif // OPTIONS_H
