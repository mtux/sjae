#include "startupstatus.h"
#include <QtPlugin>
#include <QSettings>

PluginInfo info = {
	0x1000,
	"Startup Status",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Startup status plugin for SJC",
	0x00000001
};


StartupStatus::StartupStatus()
{

}

StartupStatus::~StartupStatus()
{

}

bool StartupStatus::load(CoreI *core) {
	core_i = core;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	connect(accounts_i, SIGNAL(account_added(const QString &, const QString &)), this, SLOT(account_added(const QString &, const QString &)));
	connect(accounts_i, SIGNAL(account_removed(const QString &, const QString &)), this, SLOT(account_removed(const QString &, const QString &)));

	QStringList proto_names = accounts_i->protocol_names();
	for(int i = 0; i < proto_names.size(); i++) {
		ProtocolI *proto = accounts_i->get_proto_interface(proto_names.at(i));
		connect(proto, SIGNAL(local_status_change(const QString &, const QString &, GlobalStatus)), this, SLOT(local_status_change(const QString &, const QString &, GlobalStatus)));
	}
	return true;
}

bool StartupStatus::modules_loaded() {
	QSettings settings;
	int size = settings.beginReadArray("StartupStatus");
	for(int i = 0; i < size; i++) {
		settings.setArrayIndex(i);
		QString proto = settings.value("protocol").toString(),
			account_id = settings.value("account_id").toString();
		GlobalStatus gs = (GlobalStatus)settings.value("status").toInt();
		if(accounts_i->account_ids(proto).contains(account_id))
			accounts_i->get_proto_interface(proto)->set_status(account_id, gs);
	}
	settings.endArray();
	return true;
}

bool StartupStatus::pre_shutdown() {
	QSettings settings;
	settings.beginWriteArray("StartupStatus");
	QMapIterator<QString, QMap<QString, GlobalStatus> > i(statuses);
	int index = 0;
	while(i.hasNext()) {
		i.next();
		QMapIterator<QString, GlobalStatus> j(i.value());
		while(j.hasNext()) {
			j.next();
			settings.setArrayIndex(index);
			settings.setValue("protocol", i.key());
			settings.setValue("account_id", j.key());
			settings.setValue("status", j.value());
			index++;
		}
	}
	settings.endArray();
	return true;
}

bool StartupStatus::unload() {
	return true;
}

const PluginInfo &StartupStatus::get_plugin_info() {
	return info;
}


///////////////////////////////////

void StartupStatus::local_status_change(const QString &proto_name, const QString &account_id, GlobalStatus gs) {
	if(gs != ST_CONNECTING) statuses[proto_name][account_id] = gs;
}

void StartupStatus::account_removed(const QString &proto_name, const QString &id) {
	statuses[proto_name].remove(id);
	if(statuses[proto_name].size() == 0) statuses.remove(proto_name);
}

void StartupStatus::account_added(const QString &proto_name, const QString &id) {
	ProtocolI *proto = accounts_i->get_proto_interface(proto_name);
	connect(proto, SIGNAL(local_status_change(const QString &, const QString &, GlobalStatus)), this, SLOT(local_status_change(const QString &, const QString &, GlobalStatus)));
	GlobalStatus gs = proto->get_status(id);
	if(gs != ST_CONNECTING) statuses[proto_name][id] = gs;
}

/////////////////////////////

Q_EXPORT_PLUGIN2(startupStatus, StartupStatus)
