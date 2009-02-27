#include "events.h"
#include <QtPlugin>
#include <QDebug>
#include <QMutexLocker>

PluginInfo info = {
	0x020,
	"Events",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Events",
	0x00000001
};

//////////////////////////////////////////////////////////////

void Events::fire_event(Event &e) {
	lmutex.lock();
	EventFilterList f = filters[e.uuid];
	f += filters[QUuid()];
	EventListenerList l = listeners[e.uuid];
	l += listeners[QUuid()]; 
	lmutex.unlock();

	int m;
	switch(e.type) {
		case ET_INTERNAL: m = EVENT_TYPE_MASK_INTERNAL; break;
		case ET_INCOMMING: m = EVENT_TYPE_MASK_INCOMMING; break;
		case ET_OUTGOING: m = EVENT_TYPE_MASK_OUTGOING; break;
		default:
			m = EVENT_TYPE_MASK_ALL;
	};

	foreach(FilterInfo fi, f) {
		if(fi.mask & m)
			if(!fi.filter->event_fired(e)) return;
	}

	foreach(ListenerInfo li, l) {
		if(li.mask & m)
			li.listener->event_fired(e);
	}
}

void Events::add_event_listener(EventListener *l, const QUuid &id, int mask) {
	QMutexLocker locker(&lmutex);
	listeners[id].append(ListenerInfo(l, mask));
}

void Events::add_event_filter(EventListener *l, int order, const QUuid &id, int mask) {
	QMutexLocker locker(&lmutex);
	filters[id].append(FilterInfo(l, order, mask));
	qSort(filters[id]);
}

void Events::remove_event_listener(EventListener *l, const QUuid &id) {
	QMutexLocker locker(&lmutex);
	if(listeners.contains(id)) {
		for(int i = 0; i < listeners[id].size();) {
			if(listeners[id].at(i).listener == l)
				listeners[id].removeAt(i);
			else
				i++;
		}
		if(listeners[id].size() == 0) listeners.remove(id);
	}
}

void Events::remove_event_filter(EventListener *l, const QUuid &id) {
	QMutexLocker locker(&lmutex);
	if(filters.contains(id)) {
		for(int i = 0; i < filters[id].size();) {
			if(filters[id].at(i).filter == l)
				filters[id].removeAt(i);
			else
				i++;
		}
		if(filters[id].size() == 0) filters.remove(id);
	}
}

//////////////////////////////////////////////////////////////

Events::Events()
{

}

Events::~Events()
{

}

bool Events::load(CoreI *core) {
	core_i = core;
	return true;
}

bool Events::modules_loaded() {
	return true;
}

bool Events::pre_shutdown() {
	return true;
}

bool Events::unload() {
	return true;
}

const PluginInfo &Events::get_plugin_info() {
	return info;
}

/////////////////////////////



/////////////////////////////

Q_EXPORT_PLUGIN2(events, Events)

