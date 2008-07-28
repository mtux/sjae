#ifndef PROTOOPTIONS_H
#define PROTOOPTIONS_H

#include <accounts_i.h>
#include "ui_protooptions.h"
#include "jabberctx.h"

class ProtoOptions : public AccountExtra
{
	Q_OBJECT

public:
	ProtoOptions(QWidget *parent = 0);
	~ProtoOptions();

	void setContext(JabberCtx *c);
	void set_account_info(const AccountInfo &info);
	bool apply();
	void reset();

private:
	Ui::ProtoOptionsClass ui;
	JabberCtx *ctx;

private slots:
	void on_edConHost_textChanged(const QString &);
	void on_chkSSL_stateChanged(int);
};

#endif // PROTOOPTIONS_H
