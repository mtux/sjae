#include "asksubscribe.h"

AskSubscribe::AskSubscribe(QWidget *parent)
: QDialog(parent)
{
	ui.setupUi(this);

	connect(this, SIGNAL(accepted()), this, SLOT(emitGrantsAndClear()));
	connect(this, SIGNAL(rejected()), this, SLOT(clearList()));
}

AskSubscribe::~AskSubscribe()
{

}

void AskSubscribe::emitGrantsAndClear() {
	int rows = ui.lstUsers->count();
	for(int i = 0; i < rows; i++) {
		if(ui.lstUsers->item(i)->checkState() == Qt::Checked)
			emit grant(ui.lstUsers->item(i)->text(), ui.lstUsers->item(i)->data(Qt::UserRole).toString());
	}

	clearList();
}

void AskSubscribe::clearList() {
	ui.lstUsers->clear();
	hide();
}

void AskSubscribe::on_btnSelectAll_clicked()
{
	int rows = ui.lstUsers->count();
	for(int i = 0; i < rows; i++)
		ui.lstUsers->item(i)->setCheckState(Qt::Checked);
}

void AskSubscribe::on_btnInvertSelection_clicked()
{
	int rows = ui.lstUsers->count();
	for(int i = 0; i < rows; i++)
		ui.lstUsers->item(i)->setCheckState(ui.lstUsers->item(i)->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);

}

void AskSubscribe::addUser(const QString &jid, const QString &account_id) {
	QListWidgetItem * item = new QListWidgetItem(jid, ui.lstUsers);
	item->setData(Qt::UserRole, account_id);
	item->setCheckState(Qt::Checked);

	show();
	activateWindow();
}

