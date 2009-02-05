#include "contactlist.h"
#include <options_i.h>
#include "clistoptions.h"
#include <QSettings>
#include <QStack>
#include <QDebug>
#include <QtPlugin>
#include <QMutexLocker>

PluginInfo info = {
	0x400,
	"Contact List",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Contact List",
	0x00000001
};

ContactList::ContactList() {
	QSettings settings;
	hide_offline = settings.value("CList/hide_offline", true).toBool();

	qRegisterMetaType<ContactInfo>("ContactInfo");
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
	return true;
}

bool ContactList::unload() {
	win->deleteLater();
	return true;
}

const PluginInfo &ContactList::get_plugin_info() {
	return info;
}

/////////////////////////////

SortedTreeWidgetItem::SortedTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type): QTreeWidgetItem(parent, strings, type) {}
SortedTreeWidgetItem::SortedTreeWidgetItem(const QStringList &strings, int type): QTreeWidgetItem(strings, type) {}

bool SortedTreeWidgetItem::operator<( const QTreeWidgetItem &other) const {
	
	if(type() == TWIT_GROUP && other.type() != TWIT_GROUP) return true;
	if(type() != TWIT_GROUP && other.type() == TWIT_GROUP) return false;

	if(type() == TWIT_GROUP || other.type() == TWIT_GROUP)
		return QTreeWidgetItem::operator<(other);

	ContactInfo ci = data(0, Qt::UserRole).value<ContactInfo>(),
		ci_other = other.data(0, Qt::UserRole).value<ContactInfo>();
	//const SortedTreeWidgetItem *o = (SortedTreeWidgetItem *)(&other);
	if(ci.gs == ST_OFFLINE && ci_other.gs != ST_OFFLINE) return false;
	if(ci.gs != ST_OFFLINE && ci_other.gs == ST_OFFLINE) return true;
	if(ci.gs != ci_other.gs)
		return ci.gs < ci_other.gs;

	return QTreeWidgetItem::operator<(other);
}

/////////////////////////////

QString ContactList::make_uid(const QString &proto_name, const QString &account_id, const QString &id) {
	//QString pn(proto_name), ai(account_id), i(id);
	//QString ret = pn.replace(QString("::"), QString("<-->")) + "::" + ai.replace(QString("::"), QString("<-->")) + "::" 
	//	+ i.replace(QString("::"), QString("<-->"));
	//return ret;
	return (proto_name + account_id + id);
}

/*
QStringList ContactList::break_id(const QString &id) {
	QStringList l = id.split("::"), ret;
	foreach(QString s, l) {
		ret << s.replace("<-->", "::");
	}
	return ret;
}
*/

QTreeWidgetItem *findGroup(QTreeWidgetItem *parent, const QString &name) {
	for(int i = 0; i < parent->childCount(); i++) {
		if(parent->child(i)->type() == TWIT_GROUP && parent->child(i)->text(0) == name)
			return parent->child(i);
	}
	return 0;
}

QTreeWidgetItem *ContactList::add_contact(const QString &proto_name, const QString &account_id, const QString &id, const QString &label, GlobalStatus gs, const QString &group) {
	SortedTreeWidgetItem *si = 0;

	{ // scope locker
		QMutexLocker locker(&list_mutex);

		ContactInfo ci;
		ci.proto_name = proto_name;
		ci.account_id = account_id;
		ci.id = id;
		ci.group = group;
		ci.label = label;
		ci.gs = gs;
		ci.parent = 0;

		if(group_delim.contains(proto_name) == false || group_delim[proto_name].contains(account_id) == false)
			group_delim[proto_name][account_id] = "\\";

		QSettings settings;
		QTreeWidgetItem *i, *parent = win->tree()->invisibleRootItem();
		QString full_gn;
		if(!group.isEmpty()) {
			QStringList subgroups = group.split(group_delim[proto_name][account_id]);
			while(subgroups.size() && (i = findGroup(parent, subgroups.at(0))) != 0) {
				parent = i;
				full_gn += group_delim[proto_name][account_id] + subgroups.at(0);
				subgroups.removeAt(0);
			}
			while(subgroups.size()) {
				i = new SortedTreeWidgetItem(parent, QStringList() << subgroups.at(0) << proto_name << account_id, TWIT_GROUP);
				//parent->sortChildren(0, Qt::AscendingOrder);
				parent->setExpanded(settings.value("CList/group_expand" + full_gn, true).toBool());
				//if(hide_offline) set_hide_offline(parent);
				parent = i;
				full_gn += group_delim[proto_name][account_id] + subgroups.at(0);
				subgroups.removeAt(0);
			}
		}
			
		si = new SortedTreeWidgetItem(parent, QStringList() << label, TWIT_CONTACT);
		ci.item = si;

		QVariant var; var.setValue(ci);
		si->setData(0, Qt::UserRole, var);
		if(parent && !full_gn.isEmpty())
			parent->setExpanded(settings.value("CList/group_expand" + full_gn, true).toBool());

		si->setIcon(0, icons_i->get_account_status_icon(accounts_i->get_proto_interface(proto_name), account_id, gs));
		id_item_map[make_uid(proto_name, account_id, id)] = si;
	}

	update_hide_offline();

	return si;
}

QAction *ContactList::add_contact_action(const QString &proto_name, const QString &account_id, const QString &label, const QString &icon) {
	QMutexLocker locker(&list_mutex);

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
	{
		QMutexLocker locker(&list_mutex);

		QString cid = make_uid(proto_name, account_id, id);
		if(id_item_map.contains(cid)) {
			QSettings settings;
			QTreeWidgetItem *i = id_item_map[cid];
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

	update_hide_offline();
}

void ContactList::get_all_contacts(QTreeWidgetItem *root, QList<QString> &cids, const QString &proto_name, const QString &account_id) {
	for(int i = 0; i < root->childCount(); i++) {
		if(root->child(i)->type() == TWIT_CONTACT) {
			ContactInfo ci = root->child(i)->data(0, Qt::UserRole).value<ContactInfo>();
			if(ci.proto_name == proto_name && ci.account_id == account_id)
				cids << make_uid(proto_name, account_id, ci.id);
		} else {
			get_all_contacts(root->child(i), cids, proto_name, account_id);
		}
	}
}

void ContactList::remove_all_contacts(const QString &proto_name, const QString &account_id) {
	{
		QMutexLocker locker(&list_mutex);

		QList<QString> cids;
		get_all_contacts(win->tree()->invisibleRootItem(), cids, proto_name, account_id);

		foreach(QString cid, cids) {
			if(id_item_map.contains(cid)) {
				QSettings settings;
				QTreeWidgetItem *i = id_item_map[cid];
				id_item_map.remove(cid);

				QString full_gn;
				while(i->parent() && i->parent()->childCount() == 1)
					i = i->parent();

				delete i;
			}
		}
	}

	update_hide_offline();
}


void ContactList::set_label(const QString &proto_name, const QString &account_id, const QString &id, const QString &label) {
	{
		QMutexLocker locker(&list_mutex);

		QString cid = make_uid(proto_name, account_id, id);
		if(id_item_map.contains(cid))
			id_item_map[cid]->setText(0, label);
	}

	update_hide_offline();
}

void ContactList::set_group(const QString &proto_name, const QString &account_id, const QString &id, const QString &group) {
	list_mutex.lock();

	qDebug() << "set_group: id = " << id;
	QString cid = make_uid(proto_name, account_id, id);
	if(id_item_map.contains(cid)) {
		QTreeWidgetItem *i = id_item_map[cid];
		QIcon icon = i->icon(0);
		QString label = i->text(0);
		GlobalStatus gs = i->data(0, Qt::UserRole).value<ContactInfo>().gs;

		list_mutex.unlock();
		remove_contact(proto_name, account_id, id);
		add_contact(proto_name, account_id, id, label, gs, group);
		
		//i = id_item_map[cid].item;
		//i->setIcon(0, icon);
	} else
		list_mutex.unlock();
}

void ContactList::set_status(const QString &proto_name, const QString &account_id, const QString &id, GlobalStatus gs) {
	{ // scope locker
		QMutexLocker locker(&list_mutex);

		QString cid = make_uid(proto_name, account_id, id);
		if(id_item_map.contains(cid)) {
			qDebug() << "Setting status for id" << id << "to" << (int)gs;
			ContactInfo ci = id_item_map[cid]->data(0, Qt::UserRole).value<ContactInfo>();
			ci.gs = gs;
			QVariant var; var.setValue(ci);
			id_item_map[cid]->setData(0, Qt::UserRole, var);
			id_item_map[cid]->setIcon(0, icons_i->get_account_status_icon(accounts_i->get_proto_interface(proto_name), account_id, gs));
		}
	}

	update_hide_offline();
}

bool allChildrenHidden(QTreeWidgetItem *item) {
	for(int i = 0; i < item->childCount(); i++) {
		if(item->child(i)->isHidden() == false)
			return false;
	}
	return true;
}

void ContactList::set_hidden(const QString &proto_name, const QString &account_id, const QString &id, bool hide) {
	QString cid = make_uid(proto_name, account_id, id);
	if(id_item_map.contains(cid)) {
		QTreeWidgetItem *i = id_item_map[cid], *p = i->parent();

		if(hide) {
			i->setHidden(true);
			while(p) {
				if(allChildrenHidden(p))
					p->setHidden(true);
				p = p->parent();
			}
		} else {
			QStack<QTreeWidgetItem *> stack;
			while(i) {
				i->setHidden(false);
				i = i->parent();
			}
		}
	}
}

void ContactList::update_hide_offline() {
	win->tree()->setUpdatesEnabled(false);
	win->tree()->sortItems(0, Qt::AscendingOrder);
	set_hide_offline(hide_offline);
	win->tree()->setUpdatesEnabled(true);
}

void ContactList::set_hide_offline(bool hide) {
	QMutexLocker locker(&list_mutex);

	QSettings settings;
	settings.setValue("CList/hide_offline", hide);
	CListI::set_hide_offline(hide);

	ContactInfo ci;
	QMapIterator<QString, SortedTreeWidgetItem *> i(id_item_map);
	while(i.hasNext()) {
		i.next();
		ci = i.value()->data(0, Qt::UserRole).value<ContactInfo>();
		set_hidden(ci.proto_name, ci.account_id, ci.id, hide && ci.gs == ST_OFFLINE);
	}
}

void ContactList::aboutToShowMenuSlot(QTreeWidgetItem *i) {
	QMutexLocker locker(&list_mutex);

	if(i->type() == TWIT_CONTACT) {
		ContactInfo ci = i->data(0, Qt::UserRole).value<ContactInfo>();
		emit aboutToShowContactMenu(ci.proto_name, ci.account_id, ci.id);
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
	QMutexLocker locker(&list_mutex);
	if(i && i->type() == TWIT_CONTACT) {
		ContactInfo ci = i->data(0, Qt::UserRole).value<ContactInfo>();
		emit contact_clicked(ci.proto_name, ci.account_id, ci.id);
	}
}

void ContactList::treeItemDoubleClicked(QTreeWidgetItem *i, int col) {
	QMutexLocker locker(&list_mutex);
	if(i && i->type() == TWIT_CONTACT) {
		ContactInfo ci = i->data(0, Qt::UserRole).value<ContactInfo>();
		emit contact_dbl_clicked(ci.proto_name, ci.account_id, ci.id);
	}
}

void ContactList::treeShowTip(QTreeWidgetItem *i, const QPoint &pos) {
	QMutexLocker locker(&list_mutex);
	if(i && i->type() == TWIT_CONTACT) {
		ContactInfo ci = i->data(0, Qt::UserRole).value<ContactInfo>();
		emit show_tip(ci.proto_name, ci.account_id, ci.id, pos);
	}
}

/////////////////////////////

Q_EXPORT_PLUGIN2(contactList, ContactList)
