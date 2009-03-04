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

	void refire_latest_events(Contact *contact, QDateTime earliest, bool mark_read = true);
	void refire_latest_events(Contact *contact, int count, bool mark_read = true);

	void refire_latest_events(QList<Contact *> contacts, QDateTime earliest, bool mark_read = true);
	void refire_latest_events(QList<Contact *> contacts, int count, bool mark_read = true);

	void mark_as_read(Contact *contact, QDateTime timestamp);
	void mark_all_as_read(Contact *contact);
	void wipe_history(Contact *contact);
	
	void enable_history(Contact *contact, bool enable);

protected:
	QList<Message> read_history(QSqlQuery &query, bool mark_read);
	void mark_as_read(Contact *contact, double timestamp);

	CoreI *core_i;
	QPointer<EventsI> events_i;
	QPointer<ContactInfoI> contact_info_i;
	QPointer<AccountsI> accounts_i;

	QSqlDatabase db;
	QSqlQuery *writeQuery, *readQueryTime, *readQueryCount;

	Contact *get_contact(const QString &contact_hash);
	QMap<QString, Contact *> hashMap;
};

#endif // HISTORY

