#ifndef _I_HISTORY_H
#define _I_HISTORY_H

#include "plugin_i.h"
#include "events_i.h"
#include "contact_info_i.h"
#include <QUuid>
#include <QDateTime>

#define INAME_HISTORY		"HistoryInterface"

class HistoryI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	const QString get_interface_name() const {return INAME_HISTORY;}

	/// get a sorted list of all messages after a certain time for the given contact
	virtual QList<Message> get_latest_events(Contact *contact, QDateTime earliest, bool mark_read = true) = 0;
	/// get a sorted list of the last n messages for the given contact, where n = count
	virtual QList<Message> get_latest_events(Contact *contact, int count, bool mark_read = true) = 0;

	/// get a sorted list of all messages after a certain time for the given list of contacts (Message.contact set to source)
	virtual QList<Message> get_latest_events(QList<Contact *> contacts, QDateTime earliest, bool mark_read = true) = 0;
	/// get a sorted list of the last n messages for the given list of contacts, where n = count (Message.contact set to source)
	virtual QList<Message> get_latest_events(QList<Contact *> contacts, int count, bool mark_read = true) = 0;

	/// mark all messages at the given timestamp as read
	virtual void mark_as_read(Contact *contact, QDateTime timestamp) = 0;
	/// mark all unread messages as read for this contact
	virtual void mark_all_as_read(Contact *contact) = 0;

	/// remove all stored history for the given contact
	virtual void wipe_history(Contact *contact) = 0;

	/// enable or disable storage of message history for the given contact
	virtual void enable_history(Contact *contact, bool enable) = 0;
};

#endif
