#ifndef __MESSAGEFORMAT_H
#define __MESSAGEFORMAT_H

#include <plugin_i.h>
#include <events_i.h>
#include <contact_info_i.h>

class MessageFormat: public PluginI, public EventsI::EventListener
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	MessageFormat();
	~MessageFormat();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool event_fired(EventsI::Event &e);
protected:
	CoreI *core_i;
	QPointer<EventsI> events_i;

	void linkUrls(QString &str);
};

#endif // MESSAGEFORMAT

