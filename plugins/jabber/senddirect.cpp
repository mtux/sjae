#include "senddirect.h"

SendDirect::SendDirect(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

SendDirect::~SendDirect()
{

}

void SendDirect::add_account(const QString &id) {
	if(ui.cmbAccount->findText(id) == -1)
		ui.cmbAccount->addItem(id);
}

void SendDirect::remove_account(const QString &id) {
	int i;
	if((i = ui.cmbAccount->findText(id)) != -1)
		ui.cmbAccount->removeItem(i);
}

void SendDirect::on_btnSend_clicked() {
	QString text = ui.plainTextEdit->document()->toPlainText();
	emit send_direct(ui.cmbAccount->currentText(), ui.plainTextEdit->document()->toPlainText());
}