#ifndef SERVICEDISCOVERY_H
#define SERVICEDISCOVERY_H

#include <QWidget>
#include <QMap>
#include "ui_servicediscovery.h"
#include "disco.h"

class ServiceDiscovery : public QWidget
{
	Q_OBJECT

public:
	ServiceDiscovery(QWidget *parent = 0);
	~ServiceDiscovery();

signals:
	void queryInfo(const QString &account_id, const QString &entity_jid, const QString &node);
public slots:
	void reset();
	void gotDiscoInfo(const DiscoInfo &info);
	void gotDiscoItems(const DiscoItems &items);

protected slots:
	void itemExpanded(QTreeWidgetItem *item);
	void itemActivated(QTreeWidgetItem *item, int col);
protected:
	Ui::ServiceDiscoveryClass ui;
};

#endif // SERVICEDISCOVERY_H
