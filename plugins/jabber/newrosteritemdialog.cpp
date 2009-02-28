#include "newrosteritemdialog.h"
#include <QAbstractButton>

NewRosterItemDialog::NewRosterItemDialog(const QString &jid, const QString &name, const QStringList &gr, QWidget *parent): QDialog(parent), group(gr)
{
	ui.setupUi(this);
	ui.edJID->setText(jid);
	ui.edName->setText(name);

	ui.edJID->setEnabled(false);

	setWindowTitle("Edit Roster Item");
}

NewRosterItemDialog::~NewRosterItemDialog()
{

}


void NewRosterItemDialog::on_edJID_textChanged(const QString &text)
{
	ui.buttonBox->buttons().at(0)->setEnabled(text.length());
}
