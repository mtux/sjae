#include "autoaway.h"
#include <QtPlugin>
#include <QSettings>
#include <QCursor>
#include <QDebug>

PluginInfo info = {
	0x600,
	"AutoAway",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"AutoAway",
	0x00000001
};

AutoAway::AutoAway()
{

}

AutoAway::~AutoAway()
{

}

bool AutoAway::load(CoreI *core) {
	core_i = core;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	return true;
}

bool AutoAway::modules_loaded() {
	options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);

	QSettings settings;
	AutoAwayOptions::Settings s;
	s.enable = settings.value("AutoAway/Enable", true).toBool();
	s.min = settings.value("AutoAway/Minutes", 15).toInt();
	s.status = (GlobalStatus)settings.value("AutoAway/Status", (int)ST_SHORTAWAY).toInt();
	s.restore = settings.value("AutoAway/Restore", true).toBool();
	
	current_settings = s;

	if(options_i) {
		opt = new AutoAwayOptions(s);
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));
		options_i->add_page("Auto Away", opt);
	}


	timer.setInterval(5000);
	connect(&timer, SIGNAL(timeout()), this, SLOT(checkIdle()));
	timer.start();

	last_mouse_pos = QCursor::pos();
	idle_time = QDateTime::currentDateTime();
	idle = false;

	return true;
}

bool AutoAway::pre_shutdown() {
	timer.stop();
	returnFromIdle();
	return true;
}

bool AutoAway::unload() {
	return true;
}

const PluginInfo &AutoAway::get_plugin_info() {
	return info;
}

/////////////////////////////

void AutoAway::options_applied() {
	QSettings settings;
	AutoAwayOptions::Settings s = opt->get_settings();
	settings.setValue("AutoAway/Enable", s.enable);
	settings.setValue("AutoAway/Minutes", s.min);
	settings.setValue("AutoAway/Status", (int)s.status);
	settings.setValue("AutoAway/Restore", s.restore);

	current_settings = s;
}

void AutoAway::checkIdle() {
	QPoint pos = QCursor::pos();
	if(pos != last_mouse_pos) {
		last_mouse_pos = pos;
		idle_time = QDateTime::currentDateTime();
		if(idle) returnFromIdle();
	} else {
		if(!idle) {
			uint secs = QDateTime::currentDateTime().toTime_t() - idle_time.toTime_t();
			if(secs > current_settings.min * 60)
				goIdle();
		}
	}
}

void AutoAway::goIdle() {
	idle = true;
	
	AutoAwayStatus aas(idle, idle_time, this);
	events_i->fire_event(aas);

	if(current_settings.enable) {
		QStringList protos = accounts_i->protocol_names();
		foreach(QString proto_name, protos) {
			QStringList account_ids = accounts_i->account_ids(proto_name);
			foreach(QString account_id, account_ids) {
				Account *acc = accounts_i->account_info(proto_name, account_id);
				if(acc->status != ST_OFFLINE && acc->status != ST_INVISIBLE) {
					if(current_settings.restore) saved_status[acc] = acc->status;
					AccountStatusReq asr(acc, current_settings.status, this);
					events_i->fire_event(asr);
				}
			}
		}
	}
}

void AutoAway::returnFromIdle() {
	idle = false;

	AutoAwayStatus aas(idle, idle_time, this);
	events_i->fire_event(aas);

	if(current_settings.enable && current_settings.restore) 
		foreach(Account *acc, saved_status.keys()) {
			AccountStatusReq asr(acc, saved_status[acc], this);
			events_i->fire_event(asr);
		}

	saved_status.clear();
}

/////////////////////////////

Q_EXPORT_PLUGIN2(autoaway, AutoAway)

