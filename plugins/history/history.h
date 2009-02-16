#ifndef __HISTORY_H
#define __HISTORY_H

#include <history_i.h>
#include <contact_info_i.h>
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

	QList<Message> get_latest_events(Contact *contact, QDateTime earliest, bool mark_read = true);
	QList<Message> get_latest_events(Contact *contact, int count, bool mark_read = true);

	void mark_as_read(Contact *contact, QDateTime timestamp);

protected:
	CoreI *core_i;
	QPointer<EventsI> events_i;
	QPointer<ContactInfoI> contact_info_i;
	QPointer<AccountsI> accounts_i;

	QSqlDatabase db;
	QSqlQuery *writeQuery, *readQueryTime, *readQueryCount;
};

#endif // HISTORY

