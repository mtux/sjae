#include "events.h"
#include <QtPlugin>
#include <QDebug>
#include <QMutexLocker>

PluginInfo info = {
	0x010,
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

	foreach(EventFilter *filter, f) {
		if(!filter->event_fired(e)) return;
	}

	foreach(EventListener *listener, l) {
		listener->event_fired(e);
	}
}

void Events::add_event_listener(EventListener *l, const QUuid &id) {
	QMutexLocker locker(&lmutex);
	listeners[id].append(l);
}

void Events::add_event_filter(EventFilter *l, const QUuid &id) {
	QMutexLocker locker(&lmutex);
	filters[id].append(l);
	qSort(filters[id]);
}

void Events::remove_event_listener(EventListener *l, const QUuid &id) {
	QMutexLocker locker(&lmutex);
	int i;
	if(listeners.contains(id) && (i = listeners[id].indexOf(l)) != -1) {
		listeners[id].removeAt(i);
		if(listeners[id].size() == 0) listeners.remove(id);
	}
}

void Events::remove_event_filter(EventFilter *l, const QUuid &id) {
	QMutexLocker locker(&lmutex);
	int i;
	if(filters.contains(id) && (i = filters[id].indexOf(l)) != -1) {
		filters[id].removeAt(i);
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

