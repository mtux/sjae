#ifndef CONTACTLIST_H
#define CONTACTLIST_H

#include <clist_i.h>
#include <main_window_i.h>
#include <icons_i.h>
#include <accounts_i.h>
#include "clistwin.h"
#include <QPointer>
#include <QMenu>
#include <QMutex>
#include <QMetaType>

class SortedTreeWidgetItem: public QTreeWidgetItem {
public:
	SortedTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type);
	SortedTreeWidgetItem(const QStringList &strings, int type);
	virtual bool operator<( const QTreeWidgetItem &other) const;
};

class ContactInfo {
public:
	//ContactInfo(): item(0), gs(ST_OFFLINE) {}

	SortedTreeWidgetItem *item, *parent;
	Contact *contact;
};

class ContactList: public CListI {
	Q_OBJECT

	friend class SortedTreeWidgetItem;
public:
	ContactList();
	virtual ~ContactList();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	QTreeWidgetItem *add_contact(Contact *contact);
	QAction *add_contact_action(Account *account, const QString &label, const QString &icon = "");

public slots:
	void set_group_delimiter(Account *account, const QString &delim) {
		group_delim[account] = delim;
	}

	void remove_contact(Contact *contact);
	void remove_all_contacts(Account *account);
	void update_label(Contact *contact);
	void update_group(Contact *contact);
	void update_status(Contact *contact);

	void set_hide_offline(bool hide);
	void update_hide_offline();

	bool event_fired(EventsI::Event &e);

signals:
	//void contact_clicked(const QString &proto_name, const QString &account_id, const QString &id);
	//void contact_dbl_clicked(const QString &proto_name, const QString &account_id, const QString &id);
	//void show_tip(const QString &proto_name, const QString &account_id, const QString &id, const QPoint &p);
	//void hide_tip();
	//void aboutToShowContactMenu(const QString &proto_name, const QString &account_id, const QString &id);
	//void aboutToShowGroupMenu(const QString &proto_name, const QString &account_id, const QString &full_gn);

protected:
	void set_hidden(Contact *contact, bool hide);

	CoreI *core_i;
	QPointer<MainWindowI> main_win_i;
	QPointer<IconsI> icons_i;
	QPointer<AccountsI> accounts_i;
	QPointer<EventsI> events_i;

	CListWin *win;

	QMap<Account *, QString> group_delim;
	QMap<Contact *, SortedTreeWidgetItem *> id_item_map;

	QMutex list_mutex;

protected slots:
	void aboutToShowMenuSlot(QTreeWidgetItem *i);

	void treeItemExpanded(QTreeWidgetItem *i);
	void treeItemCollapsed(QTreeWidgetItem *i);
	
	void treeItemClicked(QTreeWidgetItem *i, int col);
	void treeItemDoubleClicked(QTreeWidgetItem *i, int col);

	void treeShowTip(QTreeWidgetItem *i, const QPoint &pos);
	void treeHideTip();
};

Q_DECLARE_METATYPE(ContactInfo)

#endif // CONTACTLIST_H
