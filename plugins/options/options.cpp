#include "options.h"
#include <QtPlugin>

PluginInfo info = {
	0x400,
	"Options",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Options",
	0x00000001
};


options::options()
{

}

options::~options()
{

}

bool options::load(CoreI *core) {
	core_i = core;
	main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW);
	win = new OptionsWin();
	if(main_win_i) main_win_i->add_window(win);
	return true;
}

bool options::modules_loaded() {
	return true;
}

bool options::pre_shutdown() {
	return true;
}

bool options::unload() {
	win->deleteLater();
	return true;
}

const PluginInfo &options::get_plugin_info() {
	return info;
}

bool options::add_page(const QString &category, OptionsPageI *page) {
	if(win) return win->add_page(category, page);
	return false;
}

void options::show_options() {
	if(win) win->show();
}


Q_EXPORT_PLUGIN2(optionsPlugin, options)
