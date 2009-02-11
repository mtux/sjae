#include "startupstatus.h"
#include <QtPlugin>
#include <QSettings>
#include <QDebug>

PluginInfo info = {
	0x1000,
	"Startup Status",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Startup Status",
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
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;

	return true;
}

bool StartupStatus::modules_loaded() {
	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);

	QSettings settings;
	int size = settings.beginReadArray("StartupStatus");
	for(int i = 0; i < size; i++) {
		settings.setArrayIndex(i);
		QString proto = settings.value("protocol").toString(),
			account_id = settings.value("account_id").toString();
		GlobalStatus gs = (GlobalStatus)settings.value("status").toInt();
		if(accounts_i->account_ids(proto).contains(account_id)) {
			qDebug() << "setting account " + account_id + " to status" << gs;
			Account *acc = accounts_i->account_info(proto, account_id);
			AccountStatusReq as(acc, gs, this);
			events_i->fire_event(as);
		}
	}
	settings.endArray();
	return true;
}

bool StartupStatus::pre_shutdown() {
	events_i->remove_event_listener(this, UUID_ACCOUNT_CHANGED);

	QSettings settings;
	settings.beginWriteArray("StartupStatus");
	QMapIterator<Account *, GlobalStatus> i(statuses);
	int index = 0;
	while(i.hasNext()) {
		i.next();

		settings.setArrayIndex(index);
		settings.setValue("protocol", i.key()->proto->name());
		settings.setValue("account_id", i.key()->account_id);
		settings.setValue("status", i.value());
		index++;
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
bool StartupStatus::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_ACCOUNT_CHANGED) {
		AccountChanged &ac = static_cast<AccountChanged &>(e);
		if(ac.removed) account_removed(ac.account);
		else account_changed(ac.account);
	}
	return true;
}

void StartupStatus::account_removed(Account *account) {
	statuses.remove(account);
}

void StartupStatus::account_changed(Account *account) {
	statuses[account] = account->desiredStatus;
}

/////////////////////////////

Q_EXPORT_PLUGIN2(startupStatus, StartupStatus)
