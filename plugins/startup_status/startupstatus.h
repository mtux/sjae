#ifndef STARTUPSTATUS_H
#define STARTUPSTATUS_H

#include <accounts_i.h>
#include <global_status.h>
#include <QPointer>
#include <QMap>

class StartupStatus: public PluginI
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

protected slots:
	void local_status_change(const QString &proto_name, const QString &account_id, GlobalStatus gs);
	void account_removed(const QString &proto_name, const QString &id);
	void account_added(const QString &proto_name, const QString &id);

protected:
	CoreI *core_i;
	QPointer<AccountsI> accounts_i;

	QMap<QString, QMap<QString, GlobalStatus> > statuses;
};

#endif // STARTUPSTATUS_H
