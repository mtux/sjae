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
	"Add Contact",
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
	if((main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW)) == 0) return false;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;

	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);

	win = new SearchWindow();

	return true;
}

bool AddContact::modules_loaded() {

	main_win_i->add_window(win);
	
	return true;
}

bool AddContact::pre_shutdown() {
	events_i->remove_event_listener(this, UUID_ACCOUNT_CHANGED);
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

bool AddContact::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_ACCOUNT_CHANGED) {
		AccountChanged &ac = static_cast<AccountChanged &>(e);
		if(ac.removed)
			win->remove_account(ac.account->proto->name(), ac.account->account_id);
		else {
			if(!win->has_account(ac.account->proto->name(), ac.account->account_id)) {
				ProtoSearchWindowI *w = ac.account->proto->create_search_window();
				if(w) {
					win->add_search_window(ac.account->proto->name(), w);
					win->add_account(ac.account->proto->name(), ac.account->account_id);
				}
			}
		}
	}
	return true;
}

void AddContact::open_search_window() {
	win->show();
	win->activateWindow();
}

/////////////////////////////

Q_EXPORT_PLUGIN2(addContact, AddContact)
