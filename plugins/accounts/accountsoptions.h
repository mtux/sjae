#ifndef ACCOUNTSOPTIONS_H
#define ACCOUNTSOPTIONS_H

#include <QWidget>
#include "ui_accountsoptions.h"
#include <accounts_i.h>
#include <QPointer>
#include <QMap>
#include <QIntValidator>

class AccountsOptions : public OptionsPageI
{
	Q_OBJECT

public:
	AccountsOptions(AccountsI *acc, QWidget *parent = 0);
	~AccountsOptions();

	bool apply();
	void reset();
signals:
	void changed(bool valid = true);
	void applied();

protected:
	QPointer<AccountsI> accounts_i;
	QMap<QString, QMap<QString, QWidget *> > proto_extra_map;
	QMap<QString, QMap<QString, AccountInfo> > acc_info;
	QMap<QString, QList<QString> > deleted_ids;

	void enableAccountInfo(bool enable);
	void setAccInfo(const QString &proto, const QString &acc);
	bool isValid(const AccountInfo &info);
	void checkValid();
private:
	Ui::AccountsOptionsClass ui;
	QIntValidator *portValidator;
private slots:
	void on_cmbAccount_currentIndexChanged(QString);
	void on_edPort_textChanged(const QString &);
	void on_edHost_textChanged(const QString &);
	void on_edPass_textChanged(const QString &);
	void on_edUname_textChanged(const QString &);
	void on_edNick_textChanged(const QString &);
	void on_btnDel_clicked();
	void on_btnCreate_clicked();
};

#endif // ACCOUNTSOPTIONS_H
