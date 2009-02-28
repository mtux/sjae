#ifndef CONTACTTREEMODEL_H
#define CONTACTTREEMODEL_H

#include <QAbstractItemModel>
#include <QMimeData>
#include <contact_info_i.h>
#include <icons_i.h>
#include <events_i.h>

typedef enum {TIT_GROUP = 1, TIT_CONTACT = 2} TreeItemType;

class TreeItem {
public:
	TreeItem(TreeItem *parent = 0);
	virtual ~TreeItem();

	void appendChild(TreeItem *child);
	void removeChild(TreeItem *child);
	virtual TreeItemType type() const = 0;

	TreeItem *child(int row) const;
	int childCount() const;
	int columnCount() const;
	int row() const;
	TreeItem *parent() const;

protected:
	QList<TreeItem*> childItems;
	TreeItem *parentItem;
	QModelIndex index;
};

class TreeItemContact: public TreeItem {
public:
	TreeItemContact(Contact *c, TreeItem *parent = 0);
	virtual ~TreeItemContact();
	TreeItemType type() const {return TIT_CONTACT;}
	Contact *getContact() const {return contact;}

private:
	Contact *contact;
};

class TreeItemGroup: public TreeItem {
public:
	TreeItemGroup(const QString &n, TreeItem *parent = 0);
	virtual ~TreeItemGroup();
	TreeItemType type() const {return TIT_GROUP;}
	QString getName() const {return name;}

	int countContacts() const;
	int countOnline() const;

	TreeItemGroup *find_group_child(QString name) const;
private:
	QString name;
};

class ContactMimeData: public QMimeData {
	Q_OBJECT
public:
	ContactMimeData(): QMimeData() {}
	Contact *c;
	QStringList formats() const {return QStringList() << "application/saje-clist-item";}
	bool hasFormat(const QString &mimeType) const { return formats().contains(mimeType);}
};

class ContactTreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	ContactTreeModel(IconsI *icons, EventsI *events, QObject *parent = 0);
	~ContactTreeModel();

	bool has_contact(Contact *contact) const;
	void addContact(Contact *contact);
	void removeContact(Contact *contact);
	void remove_all_contacts(Account *account);
	void update_contact(Contact *contact);
	
	void addGroup(QStringList &full_group_name);
	void removeGroup(QStringList &full_group_name);
	int onlineCount(QStringList &full_group_name);
	int contactCount(QStringList &full_group_name);
	
	TreeItemType getType(const QModelIndex &index) const;
	Contact *getContact(const QModelIndex &index) const;
	QStringList getGroup(const QModelIndex &index) const;

	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QStringList mimeTypes() const;
	QMimeData* mimeData(const QModelIndexList &indexes) const;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
	Qt::DropActions supportedDropActions() const;

	static QString getNick(Contact *contact);
private:
	TreeItemGroup *find_group(QStringList &name, bool create = false);
	QStringList group_full_name(TreeItemGroup *g) const;
	TreeItemGroup *rootItem;
	QMap<Contact *, TreeItemContact *> contact_item_map;

	void recursive_data_change(TreeItem *child);

	QPointer<IconsI> icons_i;
	QPointer<EventsI> events_i;
};

#endif // CONTACTTREEMODEL_H
