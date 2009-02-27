#ifndef _I_AUTOAWAY_H
#define _I_AUTOAWAY_H

#include "plugin_i.h"
#include "events_i.h"
#include <QUuid>
#include <QDateTime>

#define INAME_AUTOAWAY		"AutoAwayInterface"

#define UUID_IDLE_STATUS	"{41F273DF-68A4-477e-96C8-B0F1F20260B3}"

class AutoAwayStatus: public EventsI::Event {
public:
	AutoAwayStatus(bool idle, const QDateTime &time, QObject *source = 0): EventsI::Event(UUID_IDLE_STATUS, source), isIdle(idle), idleTime(time) {}
	bool isIdle;
	QDateTime idleTime;
};

class AutoAwayI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	const QString get_interface_name() const {return INAME_AUTOAWAY;}
};

#endif
