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
	events_i = (EventsI *)core_i->get_interface(INAME_EVENTS);

	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->add_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->add_event_listener(this, UUID_MSG);

	win = new CListWin();

	connect(win->tree(), SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(treeItemExpanded(QTreeWidgetItem *)));
	connect(win->tree(), SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(treeItemCollapsed(QTreeWidgetItem *)));

	connect(win->tree(), SIGNAL(show_tip(QTreeWidgetItem *, const QPoint &)), this, SLOT(treeShowTip(QTreeWidgetItem *, const QPoint &)));
	connect(win->tree(), SIGNAL(hide_tip()), this, SLOT(treeHideTip()));

	connect(win->tree(), SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(treeItemClicked(QTreeWidgetItem *, int)));
	connect(win->tree(), SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(treeItemDoubleClicked(QTreeWidgetItem *, int)));

	if(main_win_i) main_win_i->set_central_widget(win);
	else win->show();

	return true;
}

bool ContactList::modules_loaded() {
	OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	if(options_i)
		options_i->add_page("Appearance/Contact List", new CListOptions(this));

	// test
	//add_contact("test", "test", "test", "offline");
	//add_contact_action("test action", "dot_blue");
	//remove_contact("test");
	return true;
}

bool ContactList::pre_shutdown() {
	events_i->remove_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->remove_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->remove_event_listener(this, UUID_MSG);
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
	if(ci.contact->status == ST_OFFLINE && ci_other.contact->status != ST_OFFLINE) return false;
	if(ci.contact->status != ST_OFFLINE && ci_other.contact->status == ST_OFFLINE) return true;
	if(ci.contact->status != ci_other.contact->status)
		return ci.contact->status < ci_other.contact->status;

	return QTreeWidgetItem::operator<(other);
}

/////////////////////////////

bool ContactList::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_CONTACT_CHANGED) {
		ContactChanged &cc = static_cast<ContactChanged &>(e);
		if(cc.removed) remove_contact(cc.contact);
		else {
			list_mutex.lock();
			bool new_contact = id_item_map.contains(cc.contact) == false;
			list_mutex.unlock();

			if(new_contact) add_contact(cc.contact);
			else {
				update_label(cc.contact);
				update_group(cc.contact);
				update_status(cc.contact);
			}
		}
	} else if(e.uuid == UUID_ACCOUNT_CHANGED) {
		AccountChanged &ac = static_cast<AccountChanged &>(e);
		if(ac.removed) remove_all_contacts(ac.account);
	}
	return true;
}

QTreeWidgetItem *findGroup(QTreeWidgetItem *parent, const QString &name) {
	for(int i = 0; i < parent->childCount(); i++) {
		if(parent->child(i)->type() == TWIT_GROUP && parent->child(i)->text(0) == name)
			return parent->child(i);
	}
	return 0;
}

QString ContactList::getNick(Contact *contact) {
	QString label = contact->contact_id;
	if(contact->has_property("name"))
		label = contact->get_property("name").toString();
	if(contact->has_property("nick"))
		label = contact->get_property("nick").toString();
	if(contact->has_property("handle"))
		label = contact->get_property("handle").toString();
	return label;
}

QTreeWidgetItem *ContactList::add_contact(Contact *contact) {
	SortedTreeWidgetItem *si = 0;

	{ // scope locker
		QMutexLocker locker(&list_mutex);

		ContactInfo ci;
		ci.contact = contact;
		ci.parent = 0;

		QString proto_name = contact->account->proto->name(),
			account_id = contact->account->account_id;

		if(group_delim.contains(contact->account) == false)
			group_delim[contact->account] = "\\";

		QString group = (contact->has_property("group") ? contact->get_property("group").toString() : "");

		QSettings settings;
		QTreeWidgetItem *i, *parent = win->tree()->invisibleRootItem();
		QString full_gn;
		if(!group.isEmpty()) {
			QStringList subgroups = group.split(group_delim[contact->account]);
			while(subgroups.size() && (i = findGroup(parent, subgroups.at(0))) != 0) {
				parent = i;
				if(full_gn.size())
					full_gn += group_delim[contact->account];
				full_gn += subgroups.at(0);
				subgroups.removeAt(0);
			}
			while(subgroups.size()) {
				i = new SortedTreeWidgetItem(parent, QStringList() << subgroups.at(0) << proto_name << account_id, TWIT_GROUP);
				//parent->sortChildren(0, Qt::AscendingOrder);
				parent->setExpanded(settings.value("CList/group_expand/" + full_gn, true).toBool());
				//if(hide_offline) set_hide_offline(parent);
				parent = i;
				if(full_gn.size())
					full_gn += group_delim[contact->account];
				full_gn += subgroups.at(0);
				subgroups.removeAt(0);
			}
		}
		QString label = getNick(contact);

		si = new SortedTreeWidgetItem(parent, QStringList() << label, TWIT_CONTACT);
		ci.item = si;

		QVariant var; var.setValue(ci);
		si->setData(0, Qt::UserRole, var);
		if(parent && !full_gn.isEmpty())
			parent->setExpanded(settings.value("CList/group_expand/" + full_gn, true).toBool());

		si->setIcon(0, icons_i->get_account_status_icon(contact->account, contact->status));
		id_item_map[contact] = si;
	}

	update_hide_offline();

	return si;
}

QAction *ContactList::add_contact_action(Account *account, const QString &label, const QString &icon) {
	QMutexLocker locker(&list_mutex);

	QAction *action = new QAction(QIcon(icons_i->get_icon(icon)), label, 0);
	action->setData(QVariantList() << account->proto->name() << account->account_id);
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
		if(ret.length())
			ret += group_delim;
		ret += i->text(0);
	}
	return ret;
}

void ContactList::remove_contact(Contact *contact) {
	{
		QMutexLocker locker(&list_mutex);

		if(id_item_map.contains(contact)) {
			QSettings settings;
			QTreeWidgetItem *i = id_item_map[contact];
			id_item_map.remove(contact);

			QString full_gn;
			while(i->parent() && i->parent()->childCount() == 1) {
				i = i->parent();
				full_gn = get_full_gn(i, group_delim[contact->account]);
				settings.remove("CList/group_expand/" + full_gn);
			}

			delete i;

		}
	}

	update_hide_offline();
}

void ContactList::remove_all_contacts(Account *account) {
	{
		QMutexLocker locker(&list_mutex);

		foreach(Contact *cp, id_item_map.keys()) {
			if(cp->account == account) {
				QTreeWidgetItem *i = id_item_map[cp];
				id_item_map.remove(cp);

				while(i->parent() && i->parent()->childCount() == 1)
					i = i->parent();

				delete i;
			}
		}
	}

	update_hide_offline();
}

void ContactList::update_label(Contact *contact) {
	QString label = getNick(contact);
	{

		QMutexLocker locker(&list_mutex);

		if(id_item_map.contains(contact))
			id_item_map[contact]->setText(0, label);
	}

	update_hide_offline();
}

void ContactList::update_group(Contact *contact) {
	list_mutex.lock();

	if(id_item_map.contains(contact)) {
		QTreeWidgetItem *i = id_item_map[contact];
		QString current_group = get_full_gn(i->parent()),
			new_group = (contact->has_property("group") ? contact->get_property("group").toString() : "");
		list_mutex.unlock();
		if(current_group != new_group) {
			remove_contact(contact);
			add_contact(contact);
		}
	} else
		list_mutex.unlock();
}

void ContactList::update_status(Contact *contact) {
	{ // scope locker
		QMutexLocker locker(&list_mutex);

		if(id_item_map.contains(contact)) {
			id_item_map[contact]->setIcon(0, icons_i->get_account_status_icon(contact->account, contact->status));
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

void ContactList::set_hidden(Contact *contact, bool hide) {
	if(id_item_map.contains(contact)) {
		QTreeWidgetItem *i = id_item_map[contact], *p = i->parent();

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
	QMapIterator<Contact *, SortedTreeWidgetItem *> i(id_item_map);
	while(i.hasNext()) {
		i.next();
		set_hidden(i.key(), hide && i.key()->status == ST_OFFLINE);
	}
}

void ContactList::aboutToShowMenuSlot(QTreeWidgetItem *i) {
	list_mutex.lock();
	if(i->type() == TWIT_CONTACT) {
		ContactInfo ci = i->data(0, Qt::UserRole).value<ContactInfo>();
		list_mutex.unlock();

		events_i->fire_event(ShowContactMenu(ci.contact, this));
	} else if(i->type() == TWIT_GROUP) {
		list_mutex.unlock();
		events_i->fire_event(ShowGroupMenu(get_full_gn(i), this));
	}
}

void ContactList::treeItemExpanded(QTreeWidgetItem *i) {
	QSettings settings;
	Account *acc = accounts_i->account_info(i->text(1), i->text(2));
	QString full_gn = get_full_gn(i, group_delim[acc]);
	settings.setValue("CList/group_expand/" + full_gn, true);
}

void ContactList::treeItemCollapsed(QTreeWidgetItem *i) {
	QSettings settings;
	Account *acc = accounts_i->account_info(i->text(1), i->text(2));
	QString full_gn = get_full_gn(i, group_delim[acc]);
	settings.setValue("CList/group_expand/" + full_gn, false);
}

void ContactList::treeItemClicked(QTreeWidgetItem *i, int col) {
	list_mutex.lock();
	if(i && i->type() == TWIT_CONTACT) {
		ContactInfo ci = i->data(0, Qt::UserRole).value<ContactInfo>();
		list_mutex.unlock();
		events_i->fire_event(ContactClicked(ci.contact, this));
	} else
		list_mutex.unlock();
}

void ContactList::treeItemDoubleClicked(QTreeWidgetItem *i, int col) {
	list_mutex.lock();
	if(i && i->type() == TWIT_CONTACT) {
		ContactInfo ci = i->data(0, Qt::UserRole).value<ContactInfo>();
		list_mutex.unlock();
		events_i->fire_event(ContactDblClicked(ci.contact, this));
	} else
		list_mutex.unlock();
}

void ContactList::treeShowTip(QTreeWidgetItem *i, const QPoint &pos) {
	list_mutex.lock();
	if(i && i->type() == TWIT_CONTACT) {
		ContactInfo ci = i->data(0, Qt::UserRole).value<ContactInfo>();
		list_mutex.unlock();
		events_i->fire_event(ShowTip(ci.contact, this));
		qDebug() << "Show tip for" << ci.contact->contact_id;
	} else
		list_mutex.unlock();
}

void ContactList::treeHideTip() {
	events_i->fire_event(HideTip(this));
	qDebug() << "Hide tip";
}

/////////////////////////////

Q_EXPORT_PLUGIN2(contactList, ContactList)
