#ifndef SENDDIRECT_H
#define SENDDIRECT_H

#include <QWidget>
#include "ui_senddirect.h"
#include <accounts_i.h>

class SendDirect : public QWidget
{
	Q_OBJECT

public:
	SendDirect(QWidget *parent = 0);
	~SendDirect();
	
	void add_account(Account *acc);
	void remove_account(Account *acc);

signals:
	void send_direct(const QString &account_id, const QString &text);

private:
	Ui::SendDirectClass ui;
	QList<Account *> accounts;
private slots:
	void on_btnSend_clicked();
};

#endif // SENDDIRECT_H
