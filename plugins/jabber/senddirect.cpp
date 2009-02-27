#include "senddirect.h"

SendDirect::SendDirect(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

SendDirect::~SendDirect()
{

}

void SendDirect::add_account(Account *acc) {
	if(accounts.indexOf(acc) == -1) {
		accounts.append(acc);
		ui.cmbAccount->addItem(acc->account_name);
	}
}

void SendDirect::remove_account(Account *acc) {
	int i;
	if((i = accounts.indexOf(acc)) != -1) {
		accounts.removeAt(i);
		ui.cmbAccount->removeItem(i);
	}
}

void SendDirect::on_btnSend_clicked() {
	QString text = ui.plainTextEdit->document()->toPlainText();
	emit send_direct(accounts.at(ui.cmbAccount->currentIndex())->account_id, ui.plainTextEdit->document()->toPlainText());
}