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
#include <QSortFilterProxyModel>

#include "clistoptions.h"
#include "contacttreemodel.h"

class SortedTreeModel: public QSortFilterProxyModel {
public:
	SortedTreeModel(QObject *parent = 0);
	void setModel(ContactTreeModel *model);
	void setHideOffline(bool f);

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

	bool hideOffline, hideEmptyGroups;
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

	QAction *add_contact_action(const QString &label, const QString &icon = "");
	QAction *add_group_action(const QString &label, const QString &icon = "");

public slots:
	void add_contact(Contact *contact);
	void remove_contact(Contact *contact);
	void remove_all_contacts(Account *account);
	void update_contact(Contact *contact);

	bool event_fired(EventsI::Event &e);

protected:
	ContactTreeModel *model;
	SortedTreeModel *sortedModel;

	CoreI *core_i;
	QPointer<MainWindowI> main_win_i;
	QPointer<IconsI> icons_i;
	QPointer<AccountsI> accounts_i;
	QPointer<EventsI> events_i;

	CListWin *win;

	CListOptions *opt;
	CListOptions::Settings current_settings;

	QAction *newGroupAction, *deleteGroupAction;
	QStringList menuGroup;

protected slots:
	void aboutToShowMenuSlot(const QPoint &, const QModelIndex &i);

	void rowsInserted(const QModelIndex &parent, int start, int end);
	void rowsRemoved(const QModelIndex &parent, int start, int end);

	void treeItemExpanded(const QModelIndex &i);
	void treeItemCollapsed(const QModelIndex &i);
	
	void treeItemClicked(const QModelIndex &i);
	void treeItemDoubleClicked(const QModelIndex &i);

	void treeShowTip(const QModelIndex &i, const QPoint &pos);
	void treeHideTip();

	void options_applied();

	void newGroup();
	void deleteGroup();

	void update_group_hide(QModelIndex index, bool hide);
	void update_hide_empty_groups();
};

#endif // CONTACTLIST_H
