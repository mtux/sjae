#include "servicediscovery.h"
#include <QDebug>
#include <QTreeWidgetItem>

ServiceDiscovery::ServiceDiscovery(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.itemTree->setColumnCount(3);
	ui.itemTree->setHeaderLabels(QStringList() << "Entity" << "JID" << "Node");

	connect(ui.btnReset, SIGNAL(clicked()), this, SLOT(reset()));
	connect(ui.itemTree, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(itemExpanded(QTreeWidgetItem *)));
	connect(ui.itemTree, SIGNAL(itemActivated(QTreeWidgetItem *, int)), this, SLOT(itemActivated(QTreeWidgetItem *, int)));
}

ServiceDiscovery::~ServiceDiscovery()
{

}

void ServiceDiscovery::reset() {
	QStringList hosts;
	while(ui.itemTree->topLevelItemCount()) {
		QTreeWidgetItem *i = ui.itemTree->takeTopLevelItem(0);
		hosts << i->text(0);
		delete i;
	}

	ui.itemTree->clear();

	foreach(QString host, hosts) {
		emit queryInfo(host, "");
	}
}

void ServiceDiscovery::gotDiscoInfo(const DiscoInfo &info) {
	//qDebug() << "ServiceDiscovery window got info:" << info.entity;
	QList<QTreeWidgetItem *> items = ui.itemTree->findItems(info.entity, Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
	QTreeWidgetItem *parent = 0;
	if(items.size()) parent = items.at(0);
	else {
		qDebug() << "found no parent with text=" + info.entity;
		parent = new QTreeWidgetItem(ui.itemTree->invisibleRootItem());
	}
	parent->setText(0, info.entity);
	parent->setText(1, info.entity);
	parent->setText(2, info.node);
	while(parent->childCount())
		parent->removeChild(parent->child(0));

	if(info.indentities.size()) {
		QTreeWidgetItem *iparent = new QTreeWidgetItem(parent, QStringList() << "Identities");
		iparent->setFirstColumnSpanned(true);
		foreach(Identity ident, info.indentities) {
			QTreeWidgetItem *iitem = new QTreeWidgetItem(iparent, QStringList() << ident.category + "/" + ident.type + ": " + ident.name);
			iitem->setFirstColumnSpanned(true);
		}
	}
	if(info.features.size()) {
		QTreeWidgetItem *fparent = new QTreeWidgetItem(parent, QStringList() << "Features");
		fparent->setFirstColumnSpanned(true);
		foreach(Feature feature, info.features) {
			QTreeWidgetItem *fitem = new QTreeWidgetItem(fparent, QStringList() << feature.var);
			fitem->setFirstColumnSpanned(true);
		}
	}

	//parent->setExpanded(true);
}

void ServiceDiscovery::gotDiscoItems(const DiscoItems &items) {
	//qDebug() << "ServiceDiscovery window got items:" << items.entity;
	QList<QTreeWidgetItem *> parents = ui.itemTree->findItems(items.entity, Qt::MatchExactly);
	if(!parents.size()) {
		qDebug() << "Disco items: no such entity (" + items.entity + ")";
		return;
	}

	QTreeWidgetItem *parent = parents.at(0);
	foreach(Item item, items.items) {
		QTreeWidgetItem *iitem = new QTreeWidgetItem(parent, QStringList() << (item.name.isEmpty() ? item.jid : item.name) << item.jid << item.node),
			*dummy = new QTreeWidgetItem(iitem, QStringList () << "Querying...");
		iitem->setExpanded(false);
		dummy->setFirstColumnSpanned(true);
	}
}

void ServiceDiscovery::itemExpanded(QTreeWidgetItem *item) {
	if(!item->text(1).isEmpty()) {
		if(item->childCount() == 1 && item->child(0)->text(0) == "Querying...")
			emit queryInfo(item->text(1), item->text(2));
	}
}

void ServiceDiscovery::itemActivated(QTreeWidgetItem *titem, int col) {
	if(titem->text(0) == "jabber:iq:register" || titem->text(0).startsWith("gateway/")) {
		QString itemJid = titem->parent()->parent()->text(1);
		qDebug() << "activated registration for entity: " + itemJid;
	}
}
