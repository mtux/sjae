#include "messagenotify.h"
#include <QtPlugin>

PluginInfo info = {
	0x800,
	"MessageNotify",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"MessageNotify",
	0x00000001
};

MessageNotify::MessageNotify()
{

}

MessageNotify::~MessageNotify()
{

}

bool MessageNotify::load(CoreI *core) {
	core_i = core;
	if((popup_i = (PopupI *)core_i->get_interface(INAME_POPUP)) == 0) return false;
	if((message_win_i = (MessageWindowI *)core_i->get_interface(INAME_MESSAGE_WINDOW)) == 0) return false;
	if((history_i = (HistoryI *)core_i->get_interface(INAME_HISTORY)) == 0) return false;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;

	events_i->add_event_listener(this, UUID_MSG, EVENT_TYPE_MASK_INCOMING);
	events_i->add_event_listener(this, UUID_MSG_WIN);

	PopupI::PopupClass c= popup_i->get_class("Default");
	c.name = "Message Notify";
	c.listener = this;
	c.timeout = 0;
	popup_i->register_class(c);
	return true;
}

bool MessageNotify::modules_loaded() {
	return true;
}

bool MessageNotify::pre_shutdown() {
	events_i->remove_event_listener(this, UUID_MSG);
	events_i->remove_event_listener(this, UUID_MSG_WIN);
	return true;
}

bool MessageNotify::unload() {
	return true;
}

const PluginInfo &MessageNotify::get_plugin_info() {
	return info;
}

/////////////////////////////

void MessageNotify::popup_closed(int id, PopupI::PopupDoneType done) {
	if(winMap.contains(id)) {
		if(done == PopupI::PDT_LEFT_CLICK) {
			message_win_i->open_window(winMap[id].contact);
		} else if(history_i && done == PopupI::PDT_RIGHT_CLICK) {
			history_i->mark_as_read(winMap[id].contact, winMap[id].timestamp);
		}
		winMap.remove(id);
	}
}

QString MessageNotify::getNick(Contact *contact) {
	if(contact->has_property("handle")) return contact->get_property("handle").toString();
	if(contact->has_property("nick")) return contact->get_property("nick").toString();
	if(contact->has_property("name")) return contact->get_property("name").toString();
	return contact->contact_id;
}


bool MessageNotify::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_MSG) {
		Message &m = static_cast<Message &>(e);
		if(!m.read && open_message_windows.indexOf(m.contact) == -1) {
			QString msg = m.text;
			if(msg.startsWith("/me "))
				msg.replace(0, 4, "* " + getNick(m.contact) + " ");
			winMap[popup_i->show_popup("Message Notify", "Message from " + getNick(m.contact) + ":", msg)] = m;
		}
	} else if(e.uuid == UUID_MSG_WIN) {
		MessageWinEvent &mwe = static_cast<MessageWinEvent &>(e);
		if(!mwe.removed) {
			foreach(int id, winMap.keys()) {
				if(winMap[id].contact == mwe.contact)
					popup_i->close_popup(id);
			}
			open_message_windows.append(mwe.contact);
		} else {
			int i = open_message_windows.indexOf(mwe.contact);
			if(i != -1) open_message_windows.removeAt(i);
		}
	}
	return true;
}

/////////////////////////////

Q_EXPORT_PLUGIN2(messagenotify, MessageNotify)

