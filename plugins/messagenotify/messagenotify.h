#ifndef __MESSAGENOTIFY_H
#define __MESSAGENOTIFY_H

#include <plugin_i.h>
#include <popup_i.h>
#include <events_i.h>
#include <contact_info_i.h>
#include <message_window_i.h>
#include <history_i.h>

class MessageNotify: public PluginI, PopupI::PopupListener, EventsI::EventListener
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	MessageNotify();
	~MessageNotify();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	void popup_closed(int id, PopupI::PopupDoneType done);
	bool event_fired(EventsI::Event &e);
protected:
	CoreI *core_i;
	QPointer<PopupI> popup_i;
	QPointer<MessageWindowI> message_win_i;
	QPointer<HistoryI> history_i;
	QPointer<EventsI> events_i;

	QMap<int, Message> winMap;
	QList<Contact *> open_message_windows;
};

#endif // MESSAGENOTIFY

