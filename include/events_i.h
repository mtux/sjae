#ifndef _I_EVENTS_H
#define _I_EVENTS_H

#include "plugin_i.h"
#include <QUuid>
#include <QDateTime>

#define INAME_EVENTS	"EventsInterface"

class EventsI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	class Event {
	public:
		QUuid uuid;
		QDateTime timestamp;
		QObject *source;
		Event(const QUuid &uuid, QObject *s = 0): uuid(uuid), source(s) {
			timestamp = QDateTime::currentDateTime();
		}
	};

	class EventListener {
	public:
		virtual bool event_fired(Event &e) = 0;
	};

	class EventFilter: public EventListener {
	public:
		int order;

		bool operator<(const EventFilter &other) const {return order < other.order;}
	};

	const QString get_interface_name() const {return INAME_EVENTS;}

	virtual void fire_event(Event &e) = 0;

	virtual void add_event_listener(EventListener *l, const QUuid &id = QUuid()) = 0;
	virtual void add_event_filter(EventFilter *f, const QUuid &id = QUuid()) = 0;
	virtual void remove_event_listener(EventListener *l, const QUuid &id = QUuid()) = 0;
	virtual void remove_event_filter(EventFilter *f, const QUuid &id = QUuid()) = 0;
};

#endif
