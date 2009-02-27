#ifndef __EVENTS_H
#define __EVENTS_H

#include <events_i.h>
#include <QList>
#include <QMap>
#include <QMutex>

class Events: public EventsI
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	Events();
	~Events();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	void fire_event(Event &e);

	void add_event_listener(EventListener *l, const QUuid &id = QUuid(), int mask = EVENT_TYPE_MASK_ALL);
	void add_event_filter(EventListener *l, int order = 0, const QUuid &id = QUuid(), int mask = EVENT_TYPE_MASK_ALL);

	void remove_event_listener(EventListener *l, const QUuid &id = QUuid());
	void remove_event_filter(EventListener *l, const QUuid &id = QUuid());

protected:
	class ListenerInfo {
	public:
		EventListener *listener;
		int mask;
		ListenerInfo(EventListener *l, int m): listener(l), mask(m) {}

		bool operator==(const EventListener *l) const {return listener == l;}
	};

	class FilterInfo {
	public:
		EventListener *filter;
		int mask, order;
                FilterInfo(EventListener *f, int o, int m): filter(f), mask(m), order(o) {}

		bool operator<(const FilterInfo &other) const {return order < other.order;}
	};

	CoreI *core_i;
	typedef QList<ListenerInfo> EventListenerList;
	typedef QList<FilterInfo> EventFilterList;

	QMap<QUuid, EventListenerList> listeners;
	QMap<QUuid, EventFilterList> filters;
	QMutex lmutex;

};

#endif // EVENTS

