#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include <message_window_i.h>
#include <accounts_i.h>
#include <clist_i.h>
#include <icons_i.h>
#include "splitterwin.h"

#include <QPointer>
#include <QMap>

class MessageWindow: public MessageWindowI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	MessageWindow();
	~MessageWindow();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();
protected:
	CoreI *core_i;
	QPointer<AccountsI> accounts_i;
	QPointer<CListI> clist_i;
	QPointer<IconsI> icons_i;
	
	QMap<QString, QMap<QString, QMap<QString, SplitterWin *> > > windows;
	bool window_exists(const QString &proto_name, const QString &account_id, const QString &contact_id);

	SplitterWin *get_window(const QString &proto_name, const QString &account_id, const QString &contact_id);

	unsigned long next_msg_id;
protected slots:
	void account_added(const QString &proto_name, const QString &id);
	void account_removed(const QString &proto_name, const QString &id);
	void message_recv(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg);
	void message_send(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg);
	void status_change(const QString &proto_name, const QString &account_id, const QString &contact_id, GlobalStatus gs);
public slots:
	void open_window(const QString &proto_name, const QString &account_id, const QString &contact_id);
};

#endif // MESSAGEWINDOW_H
