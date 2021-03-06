#include "main_window.h"
#include <QtPlugin>
#include <QSettings>

PluginInfo info = {
	0x210,
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
	s.close_to_tray = settings.value("MainWin/CloseToTray", false).toBool();
	s.hide_toolbar = settings.value("MainWin/HideToolbar", false).toBool();
	s.hide_frame = settings.value("MainWin/HideFrame", false).toBool();
	s.tool_window = settings.value("MainWin/ToolWindow", false).toBool();
	s.trans_percent = settings.value("MainWin/TransPercent", 0).toInt();
	s.round_corners = settings.value("MainWin/RoundCorners", false).toBool();
	s.on_top = settings.value("MainWin/OnTop", false).toBool();

	if(options_i) {
		opt = new MainWinOptions(s);
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));
		options_i->add_page("Appearance/Main Window", opt);
	}

	win->modules_loaded();
	win->set_options(s);
	win->restoreHiddenState();

	return true;
}

void main_window::options_applied() {
	QSettings settings;
	MainWinOptions::Settings s = opt->get_settings();
	settings.setValue("MainWin/CloseToTray", s.close_to_tray);
	settings.setValue("MainWin/HideToolbar", s.hide_toolbar);
	settings.setValue("MainWin/HideFrame", s.hide_frame);
	settings.setValue("MainWin/ToolWindow", s.tool_window);
	settings.setValue("MainWin/TransPercent", s.trans_percent);
	settings.setValue("MainWin/RoundCorners", s.round_corners);
	settings.setValue("MainWin/OnTop", s.on_top);

	win->set_options(s);
}

bool main_window::pre_shutdown() {
	return true;
}

bool main_window::unload() {
	delete win;
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

Q_EXPORT_PLUGIN2(mainWindow, main_window)
