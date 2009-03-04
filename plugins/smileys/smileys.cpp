#include "smileys.h"
#include <QtPlugin>
#include <QSettings>
#include <QUrl>
#include <QDebug>

PluginInfo info = {
	0x600,
	"Smileys",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Smileys",
	0x00000001
};

Smileys::Smileys()
{

}

Smileys::~Smileys()
{

}

bool Smileys::load(CoreI *core) {
	core_i = core;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	events_i->add_event_filter(this, 10, UUID_MSG, EventsI::ET_INCOMMING | EventsI::ET_OUTGOING);

	QSettings s;
	current_settings.enable = s.value("Smileys/Enable", true).toBool();
	if(s.contains("Smileys/Enable")) {
		int size = s.beginReadArray("Smileys/Substitutions");
		for (int i = 0; i < size; ++i) {
			s.setArrayIndex(i);
			current_settings.subs[s.value("subtext").toString()] = s.value("filename").toString();
		}
		s.endArray();
	} else {
		current_settings.subs[":)"] = core_i->make_absolute("smileys/smile.png");
		current_settings.subs[":("] = core_i->make_absolute("smileys/sad.png");
		current_settings.subs[":|"] = core_i->make_absolute("smileys/unimpressed.png");
		current_settings.subs[":O"] = core_i->make_absolute("smileys/surprised.png");
		current_settings.subs[":D"] = core_i->make_absolute("smileys/big_smile.png");
		current_settings.subs[";)"] = core_i->make_absolute("smileys/wink.png");
		current_settings.subs[":P"] = core_i->make_absolute("smileys/tounge.png");
		current_settings.subs[":S"] = core_i->make_absolute("smileys/ssmile.png");
	}

	OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	if(options_i) {
		opt = new SmileysOptions(current_settings);
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));
		options_i->add_page("Appearance/Smileys", opt);
	}
	return true;
}

bool Smileys::modules_loaded() {
	return true;
}

bool Smileys::pre_shutdown() {
	events_i->remove_event_filter(this);
	return true;
}

bool Smileys::unload() {
	return true;
}

const PluginInfo &Smileys::get_plugin_info() {
	return info;
}

/////////////////////////////

void Smileys::options_applied() {
	current_settings = opt->get_settings();
	QSettings s;
	s.setValue("Smileys/Enable", current_settings.enable);
	s.beginWriteArray("Smileys/Substitutions", current_settings.subs.size());
	for(int i = 0; i < current_settings.subs.size(); i++) {
		s.setArrayIndex(i);
		QString key = current_settings.subs.keys().at(i);
		s.setValue("subtext", key);
		s.setValue("filename", current_settings.subs[key]);
	}
	s.endArray();
}

bool Smileys::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_MSG) {
		Message &m = static_cast<Message &>(e);
		foreach(QString sub, current_settings.subs.keys()) {
			QString newtext =  "<img class='smiley' src='" + QUrl::fromLocalFile(current_settings.subs[sub]).toString() + "' alt='" + sub + "' />";
			qDebug() << "smileys module replacing '" + sub + "' with '" + newtext + "'";
			m.text.replace(sub, newtext);
		}
	}
	return true;
}

/////////////////////////////

Q_EXPORT_PLUGIN2(smileys, Smileys)

