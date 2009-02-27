#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <main_window_i.h>
#include "mainwin.h"

class main_window: public MainWindowI
{
public:
	main_window();
	~main_window();
	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	void set_central_widget(QWidget *w);
	void set_status_bar(QStatusBar *sb);
	void add_window(QWidget *w);
protected:
	CoreI *core_i;

	MainWin *win;

};

#endif // MAIN_WINDOW_H
