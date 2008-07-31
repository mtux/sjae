#ifndef GATEWAYLIST_H
#define GATEWAYLIST_H

#include <QWidget>
#include "ui_gatewaylist.h"

class GatewayList : public QWidget
{
	Q_OBJECT

public:
	GatewayList(QWidget *parent = 0);
	~GatewayList();

public slots:
	void add_gateway(const QString &account_id, const QString &gateway);
	void remove_account(const QString &account_id);
signals:
	void gateway_register(const QString &account_id, const QString &gateway);
	void gateway_unregister(const QString &account_id, const QString &gateway);

private:
	Ui::GatewayListClass ui;
	QMap<QString, QStringList> gateways;

private slots:
	void on_lstGateways_itemSelectionChanged();
	void on_btnUnregister_clicked();
	void on_btnRegister_clicked();
};

#endif // GATEWAYLIST_H
