#include "contacttreemodel.h"
#include <QIcon>
#include <QDebug>

TreeItem::TreeItem(TreeItem *parent) {
	parentItem = parent;
}

TreeItem::~TreeItem() {
	qDeleteAll(childItems);
}

void TreeItem::appendChild(TreeItem *child) {
	childItems.append(child);
}

void TreeItem::removeChild(TreeItem *child) {
	int i = childItems.indexOf(child);
	if(i != -1) childItems.removeAt(i);
}

TreeItem *TreeItem::child(int row) const {
	return childItems.value(row);
}

int TreeItem::childCount() const {
	return childItems.count();
}

int TreeItem::columnCount() const {
	return 1;
}

int TreeItem::row() const {
	if (parentItem)
		return parentItem->childItems.indexOf(const_cast<TreeItem *>(this));

	return 0;
}

TreeItem *TreeItem::parent() const {
	return parentItem;
}


TreeItemContact::TreeItemContact(Contact *c, TreeItem *parent): TreeItem(parent), contact(c) {
}

TreeItemContact::~TreeItemContact() {
}

TreeItemGroup::TreeItemGroup(const QString &n, TreeItem *parent): TreeItem(parent), name(n) {
}

TreeItemGroup::~TreeItemGroup() {
}

int TreeItemGroup::countContacts() const {
	int total = 0;
	foreach(TreeItem *child, childItems) {
		if(child->type() == TIT_GROUP)
			total += static_cast<TreeItemGroup *>(child)->countContacts();
		else if(child->type() == TIT_CONTACT)
			total++;
	}
	return total;
}

int TreeItemGroup::countOnline() const {
	int total = 0;
	foreach(TreeItem *child, childItems) {
		if(child->type() == TIT_GROUP)
			total += static_cast<TreeItemGroup *>(child)->countOnline();
		else if(child->type() == TIT_CONTACT) {
			Contact *contact = static_cast<TreeItemContact *>(child)->getContact();
			if(contact->status != ST_OFFLINE)
				total++;
		}
	}
	return total;
}

TreeItemGroup *TreeItemGroup::find_group_child(QString name) const {
	foreach(TreeItem *child, childItems) {
		if(child->type() == TIT_GROUP && static_cast<TreeItemGroup *>(child)->getName() == name)
			return static_cast<TreeItemGroup *>(child);
	}
	return 0;
}

QString ContactTreeModel::getNick(Contact *contact) {
	if(contact->has_property("handle")) return contact->get_property("handle").toString();
	if(contact->has_property("nick")) return contact->get_property("nick").toString();
	if(contact->has_property("name")) return contact->get_property("name").toString();
	return contact->contact_id;
}

ContactTreeModel::ContactTreeModel(IconsI *icons, EventsI *events, QObject *parent)
	: QAbstractItemModel(parent), icons_i(icons), events_i(events)
{
	rootItem = new TreeItemGroup("root");
}

ContactTreeModel::~ContactTreeModel() {
	delete rootItem;
}

QVariant ContactTreeModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole && role != Qt::DecorationRole)
		return QVariant();

	TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

	if(role == Qt::DisplayRole && index.column() == 0) {
		if(item->type() == TIT_CONTACT)
			return getNick(static_cast<TreeItemContact *>(item)->getContact());
		else if(item->type() == TIT_GROUP)
			return static_cast<TreeItemGroup *>(item)->getName();
	}
	if(icons_i && item->type() == TIT_CONTACT && role == Qt::DecorationRole && index.column() == 0) {
		Contact *contact = static_cast<TreeItemContact *>(item)->getContact();
		if(contact->has_property("PendingMsg")) 
			return QIcon(icons_i->get_icon("message"));

		return QIcon(icons_i->get_account_status_icon(contact->account, contact->status));
	}

	return QVariant();
}

Qt::ItemFlags ContactTreeModel::flags(const QModelIndex &index) const {
	if (!index.isValid())
		return Qt::ItemIsDropEnabled;

	TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
	if(item->type() == TIT_CONTACT)
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
	else if(item->type() == TIT_GROUP)
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
	return 0;
}

QVariant ContactTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
	return QVariant();
}

QModelIndex ContactTreeModel::index(int row, int column, const QModelIndex &parent) const {
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItem *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	TreeItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

QModelIndex ContactTreeModel::parent(const QModelIndex &index) const {
	if (!index.isValid())
		return QModelIndex();

	TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
	TreeItem *parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();
	
	return createIndex(parentItem->row(), 0, parentItem);
}

int ContactTreeModel::rowCount(const QModelIndex &parent) const {
	TreeItem *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	return parentItem->childCount();
}

int ContactTreeModel::columnCount(const QModelIndex &parent) const {
	if (parent.isValid())
		return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
	else
		return rootItem->columnCount();
}

void ContactTreeModel::recursive_data_change(TreeItem *child) {
	while(child) {
		QModelIndex i = (child == rootItem ? QModelIndex() : createIndex(child->row(), 0, child));
		emit dataChanged(i, i);
		child = child->parent();
	}
}

TreeItemType ContactTreeModel::getType(const QModelIndex &index) const {
	if(index.isValid()) {
		TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
		return item->type();
	}
	return TIT_GROUP;
}

Contact *ContactTreeModel::getContact(const QModelIndex &index) const {
	if(index.isValid()) {
		TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
		if(item->type() == TIT_CONTACT)
			return static_cast<TreeItemContact *>(item)->getContact();
	}
	return 0;
}

QStringList ContactTreeModel::group_full_name(TreeItemGroup *g) const {
	QStringList ret;
	while(g && g != rootItem) {
		ret.prepend(g->getName());
		g = static_cast<TreeItemGroup *>(g->parent());
	}
	return ret;
}

QStringList ContactTreeModel::getGroup(const QModelIndex &index) const {
	if(index.isValid()) {
		TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
		if(item->type() == TIT_GROUP)
			return group_full_name(static_cast<TreeItemGroup *>(item));
	}	
	return QStringList();
}

bool ContactTreeModel::has_contact(Contact *contact) const {
	return contact_item_map.contains(contact);
}

TreeItemGroup *ContactTreeModel::find_group(QStringList &names, bool create) {
	TreeItemGroup *parent = rootItem, *g;
	foreach(QString name, names) {
		g = parent->find_group_child(name);
		if(g == 0) {
			if(create) {
				//emit layoutAboutToBeChanged();
				if(parent == rootItem) beginInsertRows(QModelIndex(), parent->childCount(), parent->childCount());
				else beginInsertRows(createIndex(parent->row(), 0, parent), parent->childCount(), parent->childCount());

				g = new TreeItemGroup(name, parent);
				parent->appendChild(g);

				//emit layoutChanged();
				endInsertRows();
				
				//recursive_data_change(parent);
			} else
				return 0;
		}
		parent = g;
	}
	return parent;
}

void ContactTreeModel::addGroup(QStringList &full_name) {
	find_group(full_name, true);
}

void ContactTreeModel::addContact(Contact *contact) {
	if(contact_item_map.contains(contact)) return;

	QStringList group;
	if(contact->has_property("group")) group = contact->get_property("group").toStringList();

	TreeItem *group_item = find_group(group, true);
	//emit layoutAboutToBeChanged();
	if(group_item == rootItem) beginInsertRows(QModelIndex(), group_item->childCount(), group_item->childCount());
	else beginInsertRows(createIndex(group_item->row(), 0, group_item), group_item->childCount(), group_item->childCount());
	TreeItemContact *item = new TreeItemContact(contact, group_item);
	group_item->appendChild(item);
	contact_item_map[contact] = item;
	//emit layoutChanged();
	endInsertRows();
	//recursive_data_change(group_item);
}

void ContactTreeModel::removeContact(Contact *contact) {
	if(contact_item_map.contains(contact)) {
		TreeItemContact *item = contact_item_map[contact];
		TreeItem *group_item = item->parent();
		//emit layoutAboutToBeChanged();
		if(group_item == rootItem) beginRemoveRows(QModelIndex(), item->row(), item->row());
		else beginRemoveRows(createIndex(group_item->row(), 0, group_item), item->row(), item->row());
		group_item->removeChild(item);
		contact_item_map.remove(contact);
		delete item;
		endRemoveRows();
		//emit layoutChanged();
		//recursive_data_change(group_item);
	}
}

void ContactTreeModel::removeGroup(QStringList &full_name) {
	TreeItemGroup *g = find_group(full_name, false);
	if(g) {
		TreeItem *group_item = g->parent();
		//emit layoutAboutToBeChanged();
		if(group_item == rootItem) beginRemoveRows(QModelIndex(), g->row(), g->row());
		else beginRemoveRows(createIndex(group_item->row(), 0, group_item), g->row(), g->row());
		group_item->removeChild(g);
		delete g;
		//emit layoutChanged();
		endRemoveRows();
		//recursive_data_change(group_item);
	}
}

int ContactTreeModel::onlineCount(QStringList &full_group_name) {
	TreeItemGroup *g = find_group(full_group_name, false);
	if(g) return g->countOnline();
	return 0;
}

int ContactTreeModel::contactCount(QStringList &full_group_name) {
	TreeItemGroup *g = find_group(full_group_name, false);
	if(g) return g->countContacts();
	return 0;
}

void ContactTreeModel::remove_all_contacts(Account *account) {
	foreach(Contact *contact, contact_item_map.keys()) {
		if(contact->account == account)
			removeContact(contact);
	}
}

void ContactTreeModel::update_contact(Contact *contact) {
	if(contact_item_map.contains(contact)) {
		TreeItemContact *item = contact_item_map[contact];
		QStringList group = group_full_name(static_cast<TreeItemGroup *>(item->parent()));
		if(contact->get_property("group").toStringList() != group) {
			// group changed
			removeContact(contact);
			addContact(contact);
		} else {
			//recursive_data_change(item);
			QModelIndex index = createIndex(item->row(), 0, item);
			emit dataChanged(index, index);
		}
	}
}

QStringList ContactTreeModel::mimeTypes() const {
	return QStringList() << "application/saje-clist-item";
}

QMimeData* ContactTreeModel::mimeData(const QModelIndexList &indexes) const {
	if(indexes.size() != 1) return 0;
	Contact *c = getContact(indexes.first());
	if(c) {
		ContactMimeData *d = new ContactMimeData();
		d->c = c;
		return d;
	}
	return 0;
}

bool ContactTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
	ContactMimeData *d = qobject_cast<ContactMimeData *>(data);
	if(d) {
		QStringList group;
		QModelIndex i = (row == -1 ? parent : index(row, 0, parent));
		if(getType(i) == TIT_GROUP) group = getGroup(i);
		else group = getContact(i)->get_property("group").toStringList();
		if(group.size())
			d->c->set_property("group", group);
		else 
			d->c->remove_property("group");
		removeContact(d->c);
		addContact(d->c);
		if(events_i) events_i->fire_event(ContactChanged(d->c, this));
	}
	return true;
}

Qt::DropActions ContactTreeModel::supportedDropActions() const {
	return Qt::MoveAction;
}
