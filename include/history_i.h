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

	virtual QList<Message> get_latest_events(Contact *contact, QDateTime earliest, bool mark_read = true) = 0;
	virtual QList<Message> get_latest_events(Contact *contact, int count, bool mark_read = true) = 0;

	virtual void mark_as_read(Contact *contact, QDateTime timestamp) = 0;
};

#endif
