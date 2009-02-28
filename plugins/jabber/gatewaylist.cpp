#include "gatewaylist.h"

GatewayList::GatewayList(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

GatewayList::~GatewayList()
{

}

void GatewayList::add_gateway(const QString &account_id, const QString &gateway) {
	if(!gateways[account_id].contains(gateway)) {
		gateways[account_id].append(gateway);
		ui.lstGateways->addItem(gateway);
	}
}

void GatewayList::remove_account(const QString &account_id) {
	if(gateways.contains(account_id)) {
		gateways.remove(account_id);
		ui.lstGateways->clear();
		ui.btnRegister->setEnabled(false);
		foreach(QString account_id, gateways.keys())
			ui.lstGateways->addItems(gateways[account_id]);
	}
}


void GatewayList::on_btnRegister_clicked()
{
	QString gateway = ui.lstGateways->currentItem()->text(), account_id;
	if(!gateway.isEmpty()) {
		foreach(QString aid, gateways.keys()) {
			if(gateways[aid].contains(gateway)) {
				account_id = aid;
				break;
			}
		}
		if(!account_id.isEmpty())
			emit gateway_register(account_id, gateway);
	}
}

void GatewayList::on_btnUnregister_clicked()
{
	QString gateway = ui.lstGateways->currentItem()->text(), account_id;
	if(!gateway.isEmpty()) {
		foreach(QString aid, gateways.keys()) {
			if(gateways[aid].contains(gateway)) {
				account_id = aid;
				break;
			}
		}
		if(!account_id.isEmpty())
			emit gateway_unregister(account_id, gateway);
	}

}

void GatewayList::on_lstGateways_itemSelectionChanged()
{
	bool sel = ui.lstGateways->currentIndex().isValid();
	ui.btnRegister->setEnabled(sel);
	ui.btnUnregister->setEnabled(sel);
}
