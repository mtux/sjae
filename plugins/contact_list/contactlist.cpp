#include "contactlist.h"
#include <options_i.h>
#include "clistoptions.h"
#include <QSettings>
#include <QStack>
#include <QDebug>
#include <QtPlugin>

PluginInfo info = {
	0x400,
	"Contact List",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Contact list plugin for SJC",
	0x00000001
};

ContactList::ContactList() {
	QSettings settings;
	hide_offline = settings.value("CList/hide_offline", true).toBool();
}

ContactList::~ContactList() {

}

bool ContactList::load(CoreI *core) {
	core_i = core;
	main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW);
	icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);
	accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS);

	win = new CListWin();

	connect(win->tree(), SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(treeItemExpanded(QTreeWidgetItem *)));
	connect(win->tree(), SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(treeItemCollapsed(QTreeWidgetItem *)));

	connect(win->tree(), SIGNAL(show_tip(QTreeWidgetItem *, const QPoint &)), this, SLOT(treeShowTip(QTreeWidgetItem *, const QPoint &)));
	connect(win->tree(), SIGNAL(hide_tip()), this, SIGNAL(hide_tip()));

	connect(win->tree(), SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(treeItemClicked(QTreeWidgetItem *, int)));
	connect(win->tree(), SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(treeItemDoubleClicked(QTreeWidgetItem *, int)));

	if(main_win_i) main_win_i->set_central_widget(win);
	else win->show();

	return true;
}

bool ContactList::modules_loaded() {
	OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	if(options_i)
		options_i->add_page("User Interface/Contact List", new CListOptions(this));

	// test
	//add_contact("test", "test", "test", "offline");
	//add_contact_action("test action", "dot_blue");
	//remove_contact("test");
	return true;
}

bool ContactList::pre_shutdown() {
	win->deleteLater();
	return true;
}

bool ContactList::unload() {
	return true;
}

const PluginInfo &ContactList::get_plugin_info() {
	return info;
}

/////////////////////////////

SortedTreeWidgetItem::SortedTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type): QTreeWidgetItem(parent, strings, type) {}

bool SortedTreeWidgetItem::operator<( const QTreeWidgetItem &other) const {
	if(type() == TWIT_GROUP && other.type() != TWIT_GROUP) return true;
	if(type() != TWIT_GROUP && other.type() == TWIT_GROUP) return false;
	return QTreeWidgetItem::operator<(other);
}

/////////////////////////////

QString ContactList::make_id(const QString &proto_name, const QString &account_id, const QString &id) {
	QString pn(proto_name), ai(account_id), i(id);
	QString ret = pn.replace(QString("::"), QString("<-->")) + "::" + ai.replace(QString("::"), QString("<-->")) + "::" 
		+ i.replace(QString("::"), QString("<-->"));
	return ret;
}

QStringList ContactList::break_id(const QString &id) {
	QStringList l = id.split("::"), ret;
	foreach(QString s, l) {
		ret << s.replace("<-->", "::");
	}
	return ret;
}

QTreeWidgetItem *findGroup(QTreeWidgetItem *parent, const QString &name) {
	for(int i = 0; i < parent->childCount(); i++) {
		if(parent->child(i)->type() == TWIT_GROUP && parent->child(i)->text(0) == name)
			return parent->child(i);
	}
	return 0;
}

QTreeWidgetItem *ContactList::add_contact(const QString &proto_name, const QString &account_id, const QString &id, const QString &label, GlobalStatus gs, const QString &group) {
	if(group_delim.contains(proto_name) == false || group_delim[proto_name].contains(account_id) == false)
		group_delim[proto_name][account_id] = "\\";
	QString cid = make_id(proto_name, account_id, id);
	QSettings settings;
	QTreeWidgetItem *i, *parent = win->tree()->invisibleRootItem();
	if(!group.isEmpty()) {
		QStringList subgroups = group.split(group_delim[proto_name][account_id]);
		QString full_gn;
		while(subgroups.size() && (i = findGroup(parent, subgroups.at(0))) != 0) {
			parent = i;
			full_gn += group_delim[proto_name][account_id] + subgroups.at(0);
			subgroups.removeAt(0);
		}
		while(subgroups.size()) {
			i = new SortedTreeWidgetItem(parent, QStringList() << subgroups.at(0), TWIT_GROUP);
			parent = i;
			i->setText(1, proto_name);
			i->setText(2, account_id);
			full_gn += group_delim[proto_name][account_id] + subgroups.at(0);
			subgroups.removeAt(0);
			parent->setExpanded(settings.value("CList/group_expand" + full_gn, true).toBool());
		}
	}
		
	i = new SortedTreeWidgetItem(parent, QStringList() << label, TWIT_CONTACT);
	i->setIcon(0, icons_i->get_account_status_icon(accounts_i->get_proto_interface(proto_name), account_id, gs));
	id_item_map[cid].item = i;
	id_item_map[cid].gs = gs;
	item_id_map[i] = cid;
	set_hidden(proto_name, account_id, id, gs == ST_OFFLINE && hide_offline);
	win->tree()->sortItems(0, Qt::AscendingOrder);

	return i;
}

QAction *ContactList::add_contact_action(const QString &proto_name, const QString &account_id, const QString &label, const QString &icon) {
	QAction *action = new QAction(QIcon(icons_i->get_icon(icon)), label, 0);
	action->setData(QVariantList() << proto_name << account_id);
	win->contact_menu()->addAction(action);
	connect(win, SIGNAL(aboutToShowMenu(QTreeWidgetItem *)), this, SLOT(aboutToShowMenuSlot(QTreeWidgetItem *)));

	return action;
}

QString get_full_gn(QTreeWidgetItem *i, const QString &group_delim = "\\") {
	QString ret;
	QStack<QTreeWidgetItem *> stack;
	while(i) {
		stack.push(i);
		i = i->parent();
	}
	while(stack.size()) {
		i = stack.pop();
		ret += group_delim + i->text(0);
	}
	return ret;
}

void ContactList::remove_contact(const QString &proto_name, const QString &account_id, const QString &id) {
	QString cid = make_id(proto_name, account_id, id);
	if(id_item_map.contains(cid)) {
		QSettings settings;
		QTreeWidgetItem *i = id_item_map[cid].item;
		item_id_map.remove(i);
		id_item_map.remove(cid);

		QString full_gn;
		while(i->parent() && i->parent()->childCount() == 1) {
			i = i->parent();
			full_gn = get_full_gn(i, group_delim[proto_name][account_id]);
			settings.remove("CList/group_expand" + full_gn);
		}

		delete i;
	}
}

void ContactList::set_label(const QString &proto_name, const QString &account_id, const QString &id, const QString &label) {
	QString cid = make_id(proto_name, account_id, id);
	if(id_item_map.contains(cid)) {
		id_item_map[cid].item->setText(0, label);
		if(id_item_map[cid].item->parent())
			id_item_map[cid].item->parent()->sortChildren(0, Qt::AscendingOrder);
		else
			win->tree()->sortItems(0, Qt::AscendingOrder);
	}
}

void ContactList::set_group(const QString &proto_name, const QString &account_id, const QString &id, const QString &group) {
	qDebug() << "set_group: id = " << id;
	QString cid = make_id(proto_name, account_id, id);
	if(id_item_map.contains(cid)) {
		QTreeWidgetItem *i = id_item_map[cid].item;
		QIcon icon = i->icon(0);
		QString label = i->text(0);
		GlobalStatus gs = id_item_map[cid].gs;

		remove_contact(proto_name, account_id, id);
		add_contact(proto_name, account_id, id, label, gs, group);
		
		i = id_item_map[cid].item;
		i->setIcon(0, icon);
	}
}

void ContactList::set_status(const QString &proto_name, const QString &account_id, const QString &id, GlobalStatus gs) {
	QString cid = make_id(proto_name, account_id, id);
	if(id_item_map.contains(cid)) {
		id_item_map[cid].gs = gs;
		id_item_map[cid].item->setIcon(0, icons_i->get_account_status_icon(accounts_i->get_proto_interface(proto_name), account_id, gs));
		if(id_item_map[cid].item->parent())
			id_item_map[cid].item->parent()->sortChildren(0, Qt::AscendingOrder);
		else
			win->tree()->sortItems(0, Qt::AscendingOrder);
		set_hidden(proto_name, account_id, id, hide_offline && gs == ST_OFFLINE);
	}
}

bool allChildrenHidden(QTreeWidgetItem *item) {
	for(int i = 0; i < item->childCount(); i++) {
		if(item->child(i)->isHidden() == false)
			return false;
	}
	return true;
}

void ContactList::set_hidden(const QString &proto_name, const QString &account_id, const QString &id, bool hide) {
	QString cid = make_id(proto_name, account_id, id);
	if(id_item_map.contains(cid)) {
		QTreeWidgetItem *i = id_item_map[cid].item;
		i->setHidden(hide);
		if(hide) {
			while(i->parent() && allChildrenHidden(i->parent())) {
				i = i->parent();
				i->setHidden(true);
			}
		} else {
			while(i->parent() && i->parent()->isHidden()) {
				i = i->parent();
				i->setHidden(false);
			}
		}
	}
}

void ContactList::set_hide_offline(bool hide) {
	QSettings settings;
	settings.setValue("CList/hide_offline", hide);
	CListI::set_hide_offline(hide);
	QMapIterator<QString, ContactInfo> i(id_item_map);
	while(i.hasNext()) {
		i.next();
		QStringList dat = break_id(i.key());
		set_hidden(dat[0], dat[1], dat[2], i.value().gs == ST_OFFLINE && hide_offline);
	}
}

void ContactList::aboutToShowMenuSlot(QTreeWidgetItem *i) {
	if(i->type() == TWIT_CONTACT && item_id_map.contains(i)) {
		QStringList dat = break_id(item_id_map[i]);
		emit aboutToShowContactMenu(dat[0], dat[1], dat[2]);
	} else if(i->type() == TWIT_GROUP) {
		emit aboutToShowGroupMenu(i->text(1), i->text(2), get_full_gn(i));
	}
}

void ContactList::treeItemExpanded(QTreeWidgetItem *i) {
	QSettings settings;
	QString full_gn = get_full_gn(i, group_delim[i->text(1)][i->text(2)]);
	settings.setValue("CList/group_expand" + full_gn, true);
}

void ContactList::treeItemCollapsed(QTreeWidgetItem *i) {
	QSettings settings;
	QString full_gn = get_full_gn(i, group_delim[i->text(1)][i->text(2)]);
	settings.setValue("CList/group_expand" + full_gn, false);
}

void ContactList::treeItemClicked(QTreeWidgetItem *i, int col) {
	if(item_id_map.contains(i)) {
		QStringList dat = break_id(item_id_map[i]);
		emit contact_clicked(dat[0], dat[1], dat[2]);
	}
}

void ContactList::treeItemDoubleClicked(QTreeWidgetItem *i, int col) {
	if(item_id_map.contains(i)) {
		QStringList dat = break_id(item_id_map[i]);
		emit contact_dbl_clicked(dat[0], dat[1], dat[2]);
	}
}

void ContactList::treeShowTip(QTreeWidgetItem *i, const QPoint &pos) {
	if(i->type() == TWIT_CONTACT) {
		if(item_id_map.contains(i)) {
			QStringList dat = break_id(item_id_map[i]);
			emit show_tip(dat[0], dat[1], dat[2], pos);
		}
	}
}

/////////////////////////////

Q_EXPORT_PLUGIN2(contactList, ContactList)
