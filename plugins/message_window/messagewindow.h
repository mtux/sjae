#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include <message_window_i.h>
#include <clist_i.h>
#include <icons_i.h>
#include <events_i.h>
#include <history_i.h>
#include "splitterwin.h"
#include "messagewindowoptions.h"

#include <QPointer>
#include <QMap>

class MessageWindow: public MessageWindowI, EventsI::EventListener {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	MessageWindow();
	~MessageWindow();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool event_fired(EventsI::Event &e);
protected:
	CoreI *core_i;
	QPointer<AccountsI> accounts_i;
	QPointer<CListI> clist_i;
	QPointer<IconsI> icons_i;
	QPointer<EventsI> events_i;
	QPointer<HistoryI> history_i;
	QPointer<OptionsI> options_i;
	
	MessageWindowOptions *opt;
	MessageWindowOptions::Settings current_settings;

	QMap<Contact *, SplitterWin *> windows;
	bool window_exists(Contact *contact);

	SplitterWin *get_window(Contact *contact);

	unsigned long next_msg_id;

protected slots:
	void account_added(Account *account);
	void account_removed(Account *account);
	void message_recv(Message &m);
	void contact_change(Contact *contact);
	void destroy_window(Contact *contact);

	void options_applied();

public slots:
	void open_window(Contact *contact);
};

#endif // MESSAGEWINDOW_H
