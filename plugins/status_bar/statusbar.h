#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <icons_i.h>
#include <events_i.h>
#include <main_window_i.h>
#include <accounts_i.h>
#include <QPointer>
#include <QSignalMapper>
#include <QToolButton>
#include <QMap>

class StatusBar: public PluginI, public EventsI::EventListener
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	StatusBar();
	~StatusBar();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool event_fired(EventsI::Event &e);

protected slots:
	void actionTriggered(QObject *action);

protected:
	CoreI *core_i;
	QPointer<MainWindowI> main_win_i;
	QPointer<IconsI> icons_i;
	QPointer<AccountsI> accounts_i;
	QPointer<EventsI> events_i;

	QStatusBar *status_bar;
	QSignalMapper *menuMapper, *protoMapper;

	QMap<QString, QMap<QString, QToolButton *> > account_buttons;
};

#endif // STATUSBAR_H
