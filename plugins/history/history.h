#ifndef __HISTORY_H
#define __HISTORY_H

#include <history_i.h>
#include <accounts_i.h>
#include <QSqlDatabase>
#include <QSqlQuery>

class History: public HistoryI, public EventsI::EventListener
{
	Q_OBJECT
public:
	History();
	~History();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool event_fired(EventsI::Event &e);

	QList<Message> get_latest_events(Contact *contact, QDateTime earliest);
	QList<Message> get_latest_events(Contact *contact, int count);

protected:
	CoreI *core_i;
	QPointer<EventsI> events_i;

	QSqlDatabase db;
	QSqlQuery *writeQuery, *readQueryTime, *readQueryCount;
};

#endif // HISTORY

