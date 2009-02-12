#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <main_window_i.h>
#include "mainwin.h"
#include "mainwinoptions.h"

class main_window: public MainWindowI
{
	Q_OBJECT
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
	void manage_window_position(QWidget *w);
	void add_submenu(QMenu *menu);
protected slots:
	void options_applied();
protected:
	CoreI *core_i;
	QPointer<OptionsI> options_i;

	MainWin *win;
	MainWinOptions *opt;
};

#endif // MAIN_WINDOW_H
