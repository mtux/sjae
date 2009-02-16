#include "messagewindow.h"
#include <global_status.h>
#include <QtPlugin>
#include <QDebug>
#include <QSettings>

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

	history_i = (HistoryI *)core_i->get_interface(INAME_HISTORY);

	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((icons_i = (IconsI *)core_i->get_interface(INAME_ICONS)) == 0) return false;
	if((clist_i = (CListI *)core_i->get_interface(INAME_CLIST)) == 0) return false;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	events_i->add_event_listener(this, UUID_MSG);
	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->add_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->add_event_listener(this, UUID_CONTACT_DBL_CLICKED);
	events_i->add_event_listener(this, UUID_CONTACT_CHAT_STATE);

	QSettings settings;
	MessageWindowOptions::Settings s;
	s.show_style = (MessageWindowOptions::Settings::ShowStyleType)settings.value("MessageWindow/ShowStyle", (int)MessageWindowOptions::Settings::SS_POPUP).toInt();
	s.load_history = (MessageWindowOptions::Settings::LoadHistoryType)settings.value("MessageWindow/LoadHistory", (int)MessageWindowOptions::Settings::LH_TIME).toInt();
	s.history_count = settings.value("MessageWindow/HistoryCount", 20).toInt();
	s.history_days = settings.value("MessageWindow/HistoryDays", 7).toInt();
	s.send_chat_state = settings.value("MessageWindow/SendChatState", true).toBool();
	
	current_settings = s;

	return true;
}

bool MessageWindow::modules_loaded() {
	options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);

	if(options_i) {
		opt = new MessageWindowOptions(current_settings, (history_i != 0));
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));
		options_i->add_page("Message Window", opt);
	}
	return true;
}

bool MessageWindow::pre_shutdown() {
	events_i->remove_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->remove_event_listener(this, UUID_MSG);
	events_i->remove_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->remove_event_listener(this, UUID_CONTACT_DBL_CLICKED);
	events_i->remove_event_listener(this, UUID_CONTACT_CHAT_STATE);

	return true;
}

bool MessageWindow::unload() {
	return true;
}

const PluginInfo &MessageWindow::get_plugin_info() {
	return info;
}

/////////////////////////////
void MessageWindow::options_applied() {
	QSettings settings;
	MessageWindowOptions::Settings s = opt->get_settings();

	settings.setValue("MessageWindow/ShowStyle", (int)s.show_style);
	settings.setValue("MessageWindow/LoadHistory", (int)s.load_history);
	settings.setValue("MessageWindow/HistoryCount", s.history_count);
	settings.setValue("MessageWindow/HistoryDays", s.history_days);
	settings.setValue("MessageWindow/SendChatState", s.send_chat_state);

	current_settings = s;

	foreach(SplitterWin *win, windows.values()) {
		win->setSendChatState(s.send_chat_state);
	}
}

bool MessageWindow::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_MSG) {
		Message &m = static_cast<Message &>(e);
		if(m.data.incomming)
			message_recv(m.contact, m.data.message, m.timestamp);
	} else if(e.uuid == UUID_CONTACT_CHANGED) {
		ContactChanged &cc = static_cast<ContactChanged &>(e);
		if(window_exists(cc.contact)) {
			if(cc.removed) {
				delete windows[cc.contact];
				windows.remove(cc.contact);
			} else {
				contact_change(cc.contact);
			}
		}
	} else if(e.uuid == UUID_ACCOUNT_CHANGED) {
		AccountChanged &ac = static_cast<AccountChanged &>(e);
		if(ac.removed) account_removed(ac.account);
	} else if(e.uuid == UUID_CONTACT_DBL_CLICKED) {
		ContactDblClicked &cd = static_cast<ContactDblClicked &>(e);
		open_window(cd.contact);
	} else if(e.uuid == UUID_CONTACT_CHAT_STATE) {
		ContactChatState &cs = static_cast<ContactChatState&>(e);
		if(window_exists(cs.contact)) {
			windows[cs.contact]->setContactChatState(cs.type);
		}
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
		if(history_i && current_settings.load_history != MessageWindowOptions::Settings::LH_NONE) {
			if(current_settings.load_history == MessageWindowOptions::Settings::LH_TIME)
				win->addEvents(history_i->get_latest_events(contact, QDateTime::currentDateTime().addDays(current_settings.history_days * -1)));
			else if(current_settings.load_history == MessageWindowOptions::Settings::LH_COUNT)
				win->addEvents(history_i->get_latest_events(contact, current_settings.history_count));
		}

		win->setSendChatState(current_settings.send_chat_state);

		connect(win, SIGNAL(closed(Contact *)), this, SLOT(destroy_window(Contact *)));
	}

	return windows[contact];
}

void MessageWindow::account_added(Account *account) {
}

void MessageWindow::account_removed(Account *account) {
	foreach(Contact *c, windows.keys()) {
		if(c->account == account)
			destroy_window(c);
	}
}

void MessageWindow::message_recv(Contact *contact, const QString &msg, QDateTime &time) {
	if(current_settings.show_style != MessageWindowOptions::Settings::SS_NONE && !window_exists(contact)) {
		SplitterWin *win = get_window(contact);
		// if we're loading history, this event has been loaded already
		if(current_settings.load_history == MessageWindowOptions::Settings::LH_NONE) {
			win->msgRecv(msg, time);
			if(history_i) history_i->mark_as_read(contact, time);
		}

		if(current_settings.show_style == MessageWindowOptions::Settings::SS_POPUP) {
			win->show();
			win->activateWindow();
			win->raise();
		} else if(current_settings.show_style == MessageWindowOptions::Settings::SS_MINIMIZED) {
			win->showMinimized();
			win->activateWindow();
		}
	} else if(window_exists(contact)) {
		SplitterWin *win = get_window(contact);
		win->msgRecv(msg, time);
		if(history_i) history_i->mark_as_read(contact, time);
		win->activateWindow();
	}
}

void MessageWindow::contact_change(Contact *contact) {
	if(window_exists(contact)) {
		SplitterWin *win = get_window(contact);
		win->setWindowIcon(icons_i->get_account_status_icon(contact->account, contact->status));
		win->update_title();
	}
}

void MessageWindow::open_window(Contact *contact) {
	SplitterWin *win = get_window(contact);
	win->show();
	win->activateWindow();
	win->raise();
}

void MessageWindow::destroy_window(Contact *contact) {
	delete windows[contact];
	windows.remove(contact);
}

/////////////////////////////

Q_EXPORT_PLUGIN2(messageWindow, MessageWindow)
