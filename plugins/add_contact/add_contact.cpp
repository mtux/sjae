#include "add_contact.h"
#include <global_status.h>
#include <QtPlugin>
#include <QDebug>

PluginInfo info = {
	0x480,
	"Add Contact",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Add Contact plugin for SJC",
	0x00000001
};

AddContact::AddContact()
{

}

AddContact::~AddContact()
{

}

bool AddContact::load(CoreI *core) {
	core_i = core;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW)) == 0) return false;

	return true;
}

bool AddContact::modules_loaded() {
	//connect(accounts_i, SIGNAL(account_removed(const QString &, const QString &)), this, SLOT(account_removed(const QString &, const QString &)));

	win = new SearchWindow();
	main_win_i->add_window(win);
	
	return true;
}

bool AddContact::pre_shutdown() {
	return true;
}

bool AddContact::unload() {
	win->deleteLater();
	return true;
}

const PluginInfo &AddContact::get_plugin_info() {
	return info;
}

/////////////////////////////


void AddContact::account_added(const QString &proto_name, const QString &id) {
}

void AddContact::account_removed(const QString &proto_name, const QString &id) {
}

//void MessageWindow::open_window(const QString &proto_name, const QString &account_id, const QString &contact_id) {
//	win->show();
//	win->activateWindow();
//}

void AddContact::open_search_window() {
	win->show();
	win->activateWindow();
}

/////////////////////////////

Q_EXPORT_PLUGIN2(addContact, AddContact)
