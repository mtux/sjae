#include "messagewindow.h"
#include <global_status.h>
#include <QtPlugin>
#include <QDebug>

PluginInfo info = {
	0x480,
	"Message Window",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Message Window",
	0x00000001
};

MessageWindow::MessageWindow(): next_msg_id(1) {
}

MessageWindow::~MessageWindow() {
}

bool MessageWindow::load(CoreI *core) {
	core_i = core;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((icons_i = (IconsI *)core_i->get_interface(INAME_ICONS)) == 0) return false;
	if((clist_i = (CListI *)core_i->get_interface(INAME_CLIST)) == 0) return false;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	events_i->add_event_listener(this, UUID_MSG_RECV);
	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->add_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->add_event_listener(this, UUID_CONTACT_DBL_CLICKED);

	return true;
}

bool MessageWindow::modules_loaded() {
	return true;
}

bool MessageWindow::pre_shutdown() {
	events_i->remove_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->remove_event_listener(this, UUID_MSG_RECV);
	events_i->remove_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->remove_event_listener(this, UUID_CONTACT_DBL_CLICKED);

	return true;
}

bool MessageWindow::unload() {
	return true;
}

const PluginInfo &MessageWindow::get_plugin_info() {
	return info;
}

/////////////////////////////
bool MessageWindow::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_MSG_RECV) {
		MessageRecv &mr = static_cast<MessageRecv &>(e);
		message_recv(mr.contact, mr.message);
	} else if(e.uuid == UUID_CONTACT_CHANGED) {
		ContactChanged &cc = static_cast<ContactChanged &>(e);
		if(windows.contains(cc.contact)) {
			if(cc.removed) {
				delete windows[cc.contact];
				windows.remove(cc.contact);
			} else {
				status_change(cc.contact);
			}
		}
	} else if(e.uuid == UUID_ACCOUNT_CHANGED) {
		AccountChanged &ac = static_cast<AccountChanged &>(e);
		if(ac.removed) account_removed(ac.account);
	} else if(e.uuid == UUID_CONTACT_DBL_CLICKED) {
		ContactDblClicked &cd = static_cast<ContactDblClicked &>(e);
		open_window(cd.contact);
	}
	return true;
}


bool MessageWindow::window_exists(Contact *contact) {
	return windows.contains(contact);
}

SplitterWin *MessageWindow::get_window(Contact *contact) {
	if(!window_exists(contact)) {
		SplitterWin *win = new SplitterWin(contact, events_i);
		windows[contact] = win;

		win->setWindowIcon(icons_i->get_account_status_icon(contact->account, contact->status));

		connect(core_i, SIGNAL(styleSheetSet(const QString &)), win, SLOT(setLogStyleSheet(const QString &)));

		win->setLogStyleSheet(qApp->styleSheet());
	}

	return windows[contact];
}

void MessageWindow::account_added(Account *account) {
}

void MessageWindow::account_removed(Account *account) {
	foreach(Contact *c, windows.keys()) {
		if(c->account == account) {
			delete windows[c];
			windows.remove(c);
		}
	}
}

void MessageWindow::message_recv(Contact *contact, const QString &msg) {
	SplitterWin *win = get_window(contact);
	win->msgRecv(msg);
}

void MessageWindow::status_change(Contact *contact) {
	if(window_exists(contact)) {
		SplitterWin *win = get_window(contact);
		win->setWindowIcon(icons_i->get_account_status_icon(contact->account, contact->status));
	}
}

void MessageWindow::open_window(Contact *contact) {
	SplitterWin *win = get_window(contact);
	win->show();
	win->activateWindow();
}

/////////////////////////////

Q_EXPORT_PLUGIN2(messageWindow, MessageWindow)
