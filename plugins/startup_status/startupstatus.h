#ifndef STARTUPSTATUS_H
#define STARTUPSTATUS_H

#include <accounts_i.h>
#include <global_status.h>
#include <QPointer>
#include <QMap>

class StartupStatus: public PluginI, public EventsI::EventListener
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	StartupStatus();
	~StartupStatus();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool event_fired(EventsI::Event &e);

protected slots:
	void account_removed(Account *account);
	void account_changed(Account *account);

protected:
	CoreI *core_i;
	QPointer<AccountsI> accounts_i;
	QPointer<EventsI> events_i;

	QMap<Account *, GlobalStatus> statuses;
};

#endif // STARTUPSTATUS_H
