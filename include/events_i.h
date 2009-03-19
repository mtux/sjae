#ifndef _I_EVENTS_H
#define _I_EVENTS_H

#include "plugin_i.h"
#include <QUuid>
#include <QDateTime>

#define INAME_EVENTS	"EventsInterface"

#define EVENT_TYPE_MASK_ALL			0xffff
#define EVENT_TYPE_MASK_INTERNAL	0x0001
#define EVENT_TYPE_MASK_INCOMING	0x0002
#define EVENT_TYPE_MASK_OUTGOING	0x0004

class EventsI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	typedef enum {ET_INTERNAL=1, ET_INCOMING=2, ET_OUTGOING=3} EventType;

	class Event {
	public:
		QUuid uuid;
		QDateTime timestamp;
		QObject *source;
		EventType type;
		Event(const QUuid &uuid, QObject *s = 0, EventType t = ET_INTERNAL): uuid(uuid), source(s), type(t) {
			timestamp = QDateTime::currentDateTime();
		}

		bool operator<(const Event &other) const {
			return timestamp < other.timestamp;
		}
	};

	class EventListener {
	public:
		virtual bool event_fired(Event &e) = 0;
	};

	const QString get_interface_name() const {return INAME_EVENTS;}

	virtual void fire_event(Event &e) = 0;

	virtual void add_event_listener(EventListener *l, const QUuid &id = QUuid(), int mask = EVENT_TYPE_MASK_ALL) = 0;
	virtual void add_event_filter(EventListener *f, int order = 0x800, const QUuid &id = QUuid(), int mask = EVENT_TYPE_MASK_ALL) = 0;
	virtual void remove_event_listener(EventListener *l, const QUuid &id = QUuid()) = 0;
	virtual void remove_event_filter(EventListener *f, const QUuid &id = QUuid()) = 0;
};

#endif
