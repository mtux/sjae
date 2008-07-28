#ifndef ADDCONTACT_H
#define ADDCONTACT_H

#include <add_contact_i.h>
#include <accounts_i.h>
#include <main_window_i.h>

#include "searchwindow.h"

#include <QPointer>
#include <QMap>

class AddContact: public AddContactI {
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
	
protected:
	CoreI *core_i;
	QPointer<AccountsI> accounts_i;
	QPointer<MainWindowI> main_win_i;
	
	SearchWindow *win;
	
protected slots:
	void account_added(const QString &proto_name, const QString &id);
	void account_removed(const QString &proto_name, const QString &id);

public slots:
	void open_search_window();
};

#endif // MESSAGEWINDOW_H
