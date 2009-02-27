#include "contactlist.h"
#include <options_i.h>
#include <QSettings>
#include <QStack>
#include <QDebug>
#include <QtPlugin>
#include <QMutexLocker>
#include <QInputDialog>
//#include "../../modeltest/modeltest.h"

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
}

ContactList::~ContactList() {

}

bool ContactList::load(CoreI *core) {
	core_i = core;
	if((main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW)) == 0) return false;
	icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;

	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->add_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->add_event_listener(this, UUID_MSG, EVENT_TYPE_MASK_INCOMMING);

	model = new ContactTreeModel(icons_i, events_i, this);
	sortedModel = new SortedTreeModel(this);
	sortedModel->setDynamicSortFilter(true);
	sortedModel->setModel(model);

	//new ModelTest(sortedModel, this);
	
	win = new CListWin();
	win->tree()->setModel(sortedModel);
	win->tree()->sortByColumn(0, Qt::AscendingOrder);

	connect(sortedModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted(const QModelIndex &, int, int)));
	connect(sortedModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(rowsRemoved(const QModelIndex &, int, int)));
	connect(sortedModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));

	connect(win, SIGNAL(showMenu(const QPoint &, const QModelIndex &)), this, SLOT(aboutToShowMenuSlot(const QPoint &, const QModelIndex &)));

	connect(win->tree(), SIGNAL(expanded(const QModelIndex &)), this, SLOT(treeItemExpanded(const QModelIndex &)));
	connect(win->tree(), SIGNAL(collapsed(const QModelIndex &)), this, SLOT(treeItemCollapsed(const QModelIndex &)));

	connect(win->tree(), SIGNAL(show_tip(const QModelIndex &, const QPoint &)), this, SLOT(treeShowTip(const QModelIndex &, const QPoint &)));
	connect(win->tree(), SIGNAL(hide_tip()), this, SLOT(treeHideTip()));

	connect(win->tree(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(treeItemClicked(const QModelIndex &)));
	connect(win->tree(), SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(treeItemDoubleClicked(const QModelIndex &)));

	QSettings s;
	current_settings.hide_offline = s.value("CList/hide_offline", false).toBool();
	current_settings.hide_empty_groups = s.value("CList/hide_empty_groups", false).toBool();
	sortedModel->setHideOffline(current_settings.hide_offline);

	if(main_win_i) main_win_i->set_central_widget(win);
	else win->show();

	newGroupAction = add_group_action("New group...");
	deleteGroupAction = add_group_action("Delete group");

	connect(newGroupAction, SIGNAL(triggered()), this, SLOT(newGroup()));
	connect(deleteGroupAction, SIGNAL(triggered()), this, SLOT(deleteGroup()));

	return true;
}

bool ContactList::modules_loaded() {
	OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	if(options_i) {
		options_i->add_page("Appearance/Contact List", opt = new CListOptions(current_settings));
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));
	}

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
void ContactList::options_applied() {
	current_settings = opt->currentSettings();
	QSettings settings;
	settings.setValue("CList/hide_offline", current_settings.hide_offline);
	settings.setValue("CList/hide_empty_groups", current_settings.hide_empty_groups);

	sortedModel->setHideOffline(current_settings.hide_offline);
	update_hide_empty_groups();
}

void ContactList::newGroup() {
	bool ok;
	QString name = QInputDialog::getText(win, "New Group", "Name", QLineEdit::Normal, QString(), &ok);
	if(ok) {
		model->addGroup(menuGroup << name);
	}
}

void ContactList::deleteGroup() {
	model->removeGroup(menuGroup);
}

/////////////////////////////

bool ContactList::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_CONTACT_CHANGED) {
		ContactChanged &cc = static_cast<ContactChanged &>(e);
		if(cc.removed) remove_contact(cc.contact);
		else {
			if(!model->has_contact(cc.contact)) add_contact(cc.contact);
			else update_contact(cc.contact);
		}
	} else if(e.uuid == UUID_ACCOUNT_CHANGED) {
		AccountChanged &ac = static_cast<AccountChanged &>(e);
		if(ac.removed || ac.account->status == ST_OFFLINE) remove_all_contacts(ac.account);
	} else if(e.uuid == UUID_MSG) {
		Message &m = static_cast<Message &>(e);
		if(!m.read) {
			if(m.contact->has_property("ClistHideUntilMsg")) {
				m.contact->remove_property("CListHideUntilMsg");
			}
		}
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

QAction *ContactList::add_contact_action(const QString &label, const QString &icon) {
	QAction *action = new QAction(QIcon(icons_i->get_icon(icon)), label, 0);
	win->contact_menu()->addAction(action);

	return action;
}

QAction *ContactList::add_group_action(const QString &label, const QString &icon) {
	QAction *action = new QAction(QIcon(icons_i->get_icon(icon)), label, 0);
	win->group_menu()->addAction(action);
	return action;
}

void ContactList::add_contact(Contact *contact) {
	model->addContact(contact);
}

void ContactList::remove_contact(Contact *contact) {
	model->removeContact(contact);
}

void ContactList::remove_all_contacts(Account *account) {
	model->remove_all_contacts(account);
}

void ContactList::update_contact(Contact *contact) {
	model->update_contact(contact);
}


void ContactList::aboutToShowMenuSlot(const QPoint &pos, const QModelIndex &i) {
	Contact *contact = model->getContact(sortedModel->mapToSource(i));
	if(contact) {
		ShowContactMenu scm(contact, this);
		events_i->fire_event(scm);
		win->contact_menu()->exec(pos);
	} else {
		menuGroup = model->getGroup(sortedModel->mapToSource(i));
		int contactCount = model->contactCount(menuGroup);
		
		deleteGroupAction->setEnabled(menuGroup.size() && contactCount == 0);
		
		ShowGroupMenu sgm(menuGroup, contactCount, this);
		events_i->fire_event(sgm);
		win->group_menu()->exec(pos);
	}
}

void ContactList::update_group_hide(QModelIndex index, bool hide) {
	QModelIndex sourceIndex = sortedModel->mapToSource(index);
	QStringList gn;
	if(model->getType(sourceIndex) == TIT_GROUP) {
		gn = model->getGroup(sourceIndex);
		win->tree()->setRowHidden(index.row(), sortedModel->parent(index), hide && model->onlineCount(gn) == 0);

		for(int i = 0; i < sortedModel->rowCount(index); i++) {
			update_group_hide(sortedModel->index(i, 0, index), hide);
		}
	}
}


void ContactList::update_hide_empty_groups() {
	bool hide = current_settings.hide_empty_groups;
	for(int i = 0; i < sortedModel->rowCount(); i++) {
		update_group_hide(sortedModel->index(i, 0), hide);
	}
}

void ContactList::rowsInserted(const QModelIndex &parent, int start, int end) {
	for(int i = start; i <= end; i++) {
		QModelIndex index = sortedModel->index(i, 0, parent), sourceIndex = sortedModel->mapToSource(index);
		QStringList gn;
		if(model->getType(sourceIndex) == TIT_GROUP) {
			gn = model->getGroup(sourceIndex);

			QSettings settings;
			bool expand = settings.value("CList/group_expand/" + gn.join(">"), true).toBool();
			win->tree()->setExpanded(index, expand);
			
			win->tree()->setRowHidden(i, parent, current_settings.hide_empty_groups);
		} else if(model->getType(sourceIndex) == TIT_CONTACT) {
			QStringList gn;
			QModelIndex current = parent;
			while(current.isValid()) {
				gn = model->getGroup(sortedModel->mapToSource(current));
				bool hide = current_settings.hide_empty_groups && model->onlineCount(gn) == 0;
				win->tree()->setRowHidden(current.row(), sortedModel->parent(current), hide);
				current = sortedModel->parent(current);
			}
		}
	}
}

void ContactList::rowsRemoved(const QModelIndex &parent, int start, int end) {
	// rows already removed from source model - mappings to source invalid
	for(int i = start; i <= end; i++) {
		QModelIndex current = parent;
		while(current.isValid()) {
			if(current_settings.hide_empty_groups) {
				bool all_rows_hidden = true;
				for(int j = 0; j < sortedModel->rowCount(current); j++)
					if(!win->tree()->isRowHidden(j, current)) {
						all_rows_hidden = false;
						break;
					}
				if(all_rows_hidden)
					win->tree()->setRowHidden(current.row(), sortedModel->parent(current), current_settings.hide_empty_groups);
			}
			current = sortedModel->parent(current);
		}
	}
}

void ContactList::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
	QModelIndex index = topLeft, sourceIndex = sortedModel->mapToSource(index);
	if(model->getType(sourceIndex) == TIT_CONTACT) {
		QStringList gn;
		QModelIndex current = sortedModel->parent(index);
		while(current.isValid()) {
			gn = model->getGroup(sortedModel->mapToSource(current));
			bool hide = current_settings.hide_empty_groups && model->onlineCount(gn) == 0;
			win->tree()->setRowHidden(current.row(), sortedModel->parent(current), hide);
			current = sortedModel->parent(current);
		}
	}
}


void ContactList::treeItemExpanded(const QModelIndex &i) {
	QSettings settings;
	QStringList full_gn = model->getGroup(sortedModel->mapToSource(i));
	settings.setValue("CList/group_expand/" + full_gn.join(">"), true);
}

void ContactList::treeItemCollapsed(const QModelIndex &i) {
	QSettings settings;
	QStringList full_gn = model->getGroup(sortedModel->mapToSource(i));
	settings.setValue("CList/group_expand/" + full_gn.join(">"), false);
}

void ContactList::treeItemClicked(const QModelIndex &i) {
	Contact *contact = model->getContact(sortedModel->mapToSource(i));
	if(contact) {
		ContactClicked cc(contact, this);
		events_i->fire_event(cc);
	}
}

void ContactList::treeItemDoubleClicked(const QModelIndex &i) {
	Contact *contact = model->getContact(sortedModel->mapToSource(i));
	if(contact) {
		ContactDblClicked cdc(contact, this);
		events_i->fire_event(cdc);
	}
}

void ContactList::treeShowTip(const QModelIndex &i, const QPoint &pos) {
	Contact *contact = model->getContact(sortedModel->mapToSource(i));
	if(contact) {
		ShowTip st(contact, this);
		events_i->fire_event(st);
	}
}

void ContactList::treeHideTip() {
	HideTip ht(this);
	events_i->fire_event(ht);
}

/////////////////////////////

SortedTreeModel::SortedTreeModel(QObject *parent): QSortFilterProxyModel(parent) {
}

void SortedTreeModel::setModel(ContactTreeModel *model) {
	QSortFilterProxyModel::setSourceModel(model);
}

void SortedTreeModel::setHideOffline(bool f) {
	hideOffline = f;
	invalidate();
	sort(0, Qt::AscendingOrder);
}

bool SortedTreeModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
	ContactTreeModel *model = static_cast<ContactTreeModel *>(sourceModel());
	TreeItemType type = model->getType(model->index(source_row, 0, source_parent));
	if(type == TIT_CONTACT) {
		Contact *c = model->getContact(model->index(source_row, 0, source_parent));
		if(hideOffline && c->status == ST_OFFLINE) return false;
		if(c->has_property("CListHide")) return false;
		if(c->has_property("CListHideUntilMsg")) return false;
	}
	return true;
}

bool SortedTreeModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
	ContactTreeModel *model = static_cast<ContactTreeModel *>(sourceModel());

	TreeItemType ltype = model->getType(left), rtype = model->getType(right);
	if(ltype != rtype) return ltype < rtype;

	if(ltype == TIT_GROUP)
		return model->getGroup(left).last().localeAwareCompare(model->getGroup(right).last()) < 0;
	
	Contact *cleft = model->getContact(left),
		*cright = model->getContact(right);

	if(cleft->status != cright->status)
		return cleft->status > cright->status; // inverted on purpose

	return ContactTreeModel::getNick(cleft).localeAwareCompare(ContactTreeModel::getNick(cright)) < 0;
}

/////////////////////////////

Q_EXPORT_PLUGIN2(contactList, ContactList)
