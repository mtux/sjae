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

	void add_event_listener(EventListener *l, const QUuid &id = QUuid());
	void add_event_filter(EventFilter *l, const QUuid &id = QUuid());

	void remove_event_listener(EventListener *l, const QUuid &id = QUuid());
	void remove_event_filter(EventFilter *l, const QUuid &id = QUuid());

protected:
	CoreI *core_i;
	typedef QList<EventListener *> EventListenerList;
	typedef QList<EventFilter *> EventFilterList;

	QMap<QUuid, EventListenerList> listeners;
	QMap<QUuid, EventFilterList> filters;
	QMutex lmutex;

};

#endif // EVENTS

