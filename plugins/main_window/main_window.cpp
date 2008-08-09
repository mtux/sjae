#include "main_window.h"
#include <QtPlugin>
#include <QSettings>

PluginInfo info = {
	0x200,
	"Main Window",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Main Window",
	0x00000001
};


main_window::main_window()
{
}

main_window::~main_window()
{
}

bool main_window::load(CoreI *core) {
	core_i = core;
	win = new MainWin(core_i);

	return true;
}

bool main_window::modules_loaded() {	
	OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);

	QSettings settings;
	MainWinOptions::Settings s;
	s.hide_frame = settings.value("MainWin/HideFrame", false).toBool();
	s.trans_percent = settings.value("MainWin/TransPercent", 0).toInt();

	if(options_i) {
		opt = new MainWinOptions(s);
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));
		options_i->add_page("User Interface/Main Window", opt);
	}

	win->set_hide_frame(s.hide_frame);
	win->set_transparency(s.trans_percent);
	win->restoreHiddenState();

	return true;
}

void main_window::options_applied() {
	QSettings settings;
	MainWinOptions::Settings s = opt->get_settings();
	settings.setValue("MainWin/HideFrame", s.hide_frame);
	settings.setValue("MainWin/TransPercent", s.trans_percent);

	win->set_hide_frame(s.hide_frame);
	win->set_transparency(s.trans_percent);
}

bool main_window::pre_shutdown() {
	return true;
}

bool main_window::unload() {
	win->deleteLater();
	return true;
}

const PluginInfo &main_window::get_plugin_info() {
	return info;
}

void main_window::set_central_widget(QWidget *w) {
	if(win) win->setCentralWidget(w);
}

void main_window::set_status_bar(QStatusBar *sb) {
	if(win) win->setStatusBar(sb);
}

void main_window::add_window(QWidget *w) {	
	if(win) win->add_window(w);
}

void main_window::manage_window_position(QWidget *w) {
	if(win) win->manage_window_position(w);
}

void main_window::add_submenu(QMenu *menu) {
	if(win) win->add_submenu(menu);
}

Q_EXPORT_PLUGIN2(mainWindow, main_window)
