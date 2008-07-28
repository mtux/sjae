#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <icons_i.h>
#include <main_window_i.h>
#include <accounts_i.h>
#include <QPointer>
#include <QSignalMapper>
#include <QToolButton>
#include <QMap>

class StatusBar: public PluginI
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

protected slots:
	void actionTriggered(QObject *action);
	void local_status_change(const QString &proto_name, const QString &account_id, GlobalStatus gs);
	void account_added(const QString &proto_name, const QString &id);
	void account_removed(const QString &proto_name, const QString &id);
protected:
	CoreI *core_i;
	QPointer<MainWindowI> main_win_i;
	QPointer<IconsI> icons_i;
	QPointer<AccountsI> accounts_i;

	QStatusBar *status_bar;
	QSignalMapper *menuMapper, *protoMapper;

	QMap<QString, QMap<QString, QToolButton *> > account_buttons;
};

#endif // STATUSBAR_H
