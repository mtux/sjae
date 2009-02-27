#ifndef ADDCONTACT_H
#define ADDCONTACT_H

#include <add_contact_i.h>
#include <accounts_i.h>
#include <events_i.h>
#include <main_window_i.h>

#include "searchwindow.h"

#include <QPointer>
#include <QMap>

class AddContact: public AddContactI, public EventsI::EventListener {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	AddContact();
	~AddContact();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool event_fired(EventsI::Event &e);
	
protected:
	CoreI *core_i;
	QPointer<MainWindowI> main_win_i;
	QPointer<EventsI> events_i;
	
	SearchWindow *win;
	
public slots:
	void open_search_window();
};

#endif // MESSAGEWINDOW_H
