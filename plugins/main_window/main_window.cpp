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
	win->show();
	return true;
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

Q_EXPORT_PLUGIN2(mainWindow, main_window)