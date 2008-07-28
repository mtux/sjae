#ifndef NEWROSTERITEMDIALOG_H
#define NEWROSTERITEMDIALOG_H

#include <QDialog>
#include "ui_newrosteritemdialog.h"

class NewRosterItemDialog : public QDialog
{
	Q_OBJECT

public:
	NewRosterItemDialog(const QString &group = "", QWidget *parent = 0);
	NewRosterItemDialog(const QString &jid, const QString &name, const QString &group, QWidget *parent = 0);
	~NewRosterItemDialog();

	const QString getJID() const {return ui.edJID->text();}
	const QString getName() const {return ui.edName->text();}
	const QString getGroup() const {return ui.edGroup->text();}
private:
	Ui::NewRosterItemDialogClass ui;

private slots:
	void on_edJID_textChanged(const QString &);
};

#endif // NEWROSTERITEMDIALOG_H
