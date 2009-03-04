#ifndef __SMILEYS_H
#define __SMILEYS_H

#include <plugin_i.h>
#include <events_i.h>
#include <contact_info_i.h>
#include "smileysoptions.h"

class Smileys: public PluginI, public EventsI::EventListener
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	Smileys();
	~Smileys();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool event_fired(EventsI::Event &e);
protected slots:
	void options_applied();
protected:
	CoreI *core_i;
	QPointer<EventsI> events_i;

	SmileysOptions *opt;
	SmileysOptions::Settings current_settings;
};

#endif // SMILEYS

