#ifndef PROTOOPTIONS_H
#define PROTOOPTIONS_H

#include <accounts_i.h>
#include "ui_protooptions.h"
#include "jabberctx.h"

class ProtoOptions : public AccountExtra
{
	Q_OBJECT

public:
	ProtoOptions(ProtocolI *proto, QWidget *parent = 0);
	~ProtoOptions();

	void set_account_info(Account *acc);
	bool apply();
	void reset();

private:
	Ui::ProtoOptionsClass ui;
	ProtocolI *proto;
	Account *account;
	
private slots:
	void on_chkIgnoreSSLErrors_stateChanged(int);
	void on_edConHost_textChanged(const QString &);
	void on_chkSSL_stateChanged(int);
};

#endif // PROTOOPTIONS_H
