#include "newrosteritemdialog.h"
#include <QAbstractButton>

NewRosterItemDialog::NewRosterItemDialog(const QString &group, QWidget *parent): QDialog(parent)
{
	ui.setupUi(this);
	ui.edGroup->setText(group);

	ui.buttonBox->buttons().at(0)->setEnabled(false);
	setWindowTitle("New Roster Item");
}

NewRosterItemDialog::NewRosterItemDialog(const QString &jid, const QString &name, const QString &group, QWidget *parent): QDialog(parent)
{
	ui.setupUi(this);
	ui.edJID->setText(jid);
	ui.edName->setText(name);
	ui.edGroup->setText(group);

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