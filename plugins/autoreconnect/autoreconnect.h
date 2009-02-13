#ifndef __AUTORECONNECT_H
#define __AUTORECONNECT_H

#include <options_i.h>
#include <events_i.h>
#include <accounts_i.h>
#include "autoreconnectoptions.h"

#include <QTimer>
class AutoReconnect: public PluginI
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	AutoReconnect();
	~AutoReconnect();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

public slots:
	void options_applied();
protected slots:
	void reconnect();

protected:
	CoreI *core_i;
	QPointer<OptionsI> options_i;
	QPointer<EventsI> events_i;
	QPointer<AccountsI> accounts_i;

	QTimer timer;

	AutoReconnectOptions *opt;
	AutoReconnectOptions::Settings current_settings;
};

#endif // AUTORECONNECT

