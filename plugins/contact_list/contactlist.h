#ifndef CONTACTLIST_H
#define CONTACTLIST_H

#include <clist_i.h>
#include <main_window_i.h>
#include <icons_i.h>
#include <accounts_i.h>
#include "clistwin.h"
#include <QPointer>
#include <QMenu>

class SortedTreeWidgetItem: public QTreeWidgetItem {
public:
	SortedTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type);
	virtual bool operator<( const QTreeWidgetItem &other) const;
};

class ContactList: public CListI {
	Q_OBJECT

public:
	ContactList();
	virtual ~ContactList();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	QTreeWidgetItem *add_contact(const QString &proto_name, const QString &account_id, const QString &id, const QString &label, GlobalStatus gs, const QString &group = "");
	QAction *add_contact_action(const QString &proto_name, const QString &account_id, const QString &label, const QString &icon = "");

public slots:
	void set_group_delimiter(const QString &proto_name, const QString &account_id, const QString &delim) {
		group_delim[proto_name][account_id] = delim;
	}

	void remove_contact(const QString &proto_name, const QString &account_id, const QString &id);
	void set_label(const QString &proto_name, const QString &account_id, const QString &id, const QString &label);
	void set_group(const QString &proto_name, const QString &account_id, const QString &id, const QString &group);
	void set_status(const QString &proto_name, const QString &account_id, const QString &id, GlobalStatus gs);
	void set_hidden(const QString &proto_name, const QString &account_id, const QString &id, bool hide);

	void set_hide_offline(bool hide);

signals:
	void contact_clicked(const QString &proto_name, const QString &account_id, const QString &id);
	void contact_dbl_clicked(const QString &proto_name, const QString &account_id, const QString &id);
	void show_tip(const QString &proto_name, const QString &account_id, const QString &id, const QPoint &p);
	void hide_tip();
	//void label_changed(const QString &id, const QString &label);
	void aboutToShowContactMenu(const QString &proto_name, const QString &account_id, const QString &id);
	void aboutToShowGroupMenu(const QString &proto_name, const QString &account_id, const QString &full_gn);

protected:
	CoreI *core_i;
	QPointer<MainWindowI> main_win_i;
	QPointer<IconsI> icons_i;
	QPointer<AccountsI> accounts_i;

	CListWin *win;

	QMap<QString, QMap<QString, QString> > group_delim;

	class ContactInfo {
	public:
		QTreeWidgetItem *item;
		GlobalStatus gs;
	};
	QMap<QString, ContactInfo> id_item_map;
	QMap<QTreeWidgetItem *, QString> item_id_map;

	QString make_id(const QString &proto_name, const QString &account_id, const QString &id);
	QStringList break_id(const QString &id);

protected slots:
	void aboutToShowMenuSlot(QTreeWidgetItem *i);

	void treeItemExpanded(QTreeWidgetItem *i);
	void treeItemCollapsed(QTreeWidgetItem *i);
	
	void treeItemClicked(QTreeWidgetItem *i, int col);
	void treeItemDoubleClicked(QTreeWidgetItem *i, int col);

	void treeShowTip(QTreeWidgetItem *i, const QPoint &pos);
};

#endif // CONTACTLIST_H
