#include "autoreconnect.h"
#include <QtPlugin>
#include <QSettings>

PluginInfo info = {
	0x600,
	"AutoReconnect",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"AutoReconnect",
	0x00000001
};

AutoReconnect::AutoReconnect()
{

}

AutoReconnect::~AutoReconnect()
{

}

bool AutoReconnect::load(CoreI *core) {
	core_i = core;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	return true;
}

bool AutoReconnect::modules_loaded() {
	options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);

	QSettings settings;
	AutoReconnectOptions::Settings s;
	s.enable = settings.value("AutoReconnect/Enable", true).toBool();

	current_settings = s;

	if(options_i) {
		opt = new AutoReconnectOptions(s);
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));
		options_i->add_page("Auto Reconnect", opt);
	}


	timer.setInterval(15000);
	connect(&timer, SIGNAL(timeout()), this, SLOT(reconnect()));
	timer.start();

	return true;
}

bool AutoReconnect::pre_shutdown() {
	timer.stop();
	return true;
}

bool AutoReconnect::unload() {
	return true;
}

const PluginInfo &AutoReconnect::get_plugin_info() {
	return info;
}

/////////////////////////////

void AutoReconnect::options_applied() {
	QSettings settings;
	AutoReconnectOptions::Settings s = opt->get_settings();
	settings.setValue("AutoReconnect/Enable", s.enable);
	current_settings = s;
}

void AutoReconnect::reconnect() {
	if(current_settings.enable) {
		QStringList protos = accounts_i->protocol_names();
		foreach(QString proto_name, protos) {
			QStringList account_ids = accounts_i->account_ids(proto_name);
			foreach(QString account_id, account_ids) {
				Account *acc = accounts_i->account_info(proto_name, account_id);
				if(acc->status != acc->desiredStatus && acc->status != ST_CONNECTING) {
					events_i->fire_event(AccountStatusReq(acc, acc->desiredStatus, this));
				}
			}
		}
	}
}

/////////////////////////////

Q_EXPORT_PLUGIN2(autoreconnect, AutoReconnect)

