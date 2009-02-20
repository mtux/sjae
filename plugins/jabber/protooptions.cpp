#include "protooptions.h"
#include "jabber.h"

ProtoOptions::ProtoOptions(ProtocolI *p, QWidget *parent)
	: AccountExtra(parent), proto(p), account(0)
{
	ui.setupUi(this);
}

ProtoOptions::~ProtoOptions() {

}

void ProtoOptions::set_account_info(Account *acc) {
	account = acc;
}

bool ProtoOptions::apply() {
	if(proto && account) {
		static_cast<JabberProto *>(proto)->setUseSSL(account, ui.chkSSL->isChecked(), ui.chkIgnoreSSLErrors->isChecked());
		static_cast<JabberProto *>(proto)->setConnectionHost(account, ui.edConHost->text());
		return true;
	}
	return false;
}

void ProtoOptions::reset() {
	if(proto && account) {
		ui.chkSSL->setChecked(static_cast<JabberProto *>(proto)->getUseSSL(account));
		ui.chkIgnoreSSLErrors->setChecked(static_cast<JabberProto *>(proto)->getIgnoreSSLErrors(account));
		ui.edConHost->setText(static_cast<JabberProto *>(proto)->getConnectionHost(account));
	}
}

void ProtoOptions::on_chkSSL_stateChanged(int) {
	emit changed(true);
}


void ProtoOptions::on_edConHost_textChanged(const QString &) {
	emit changed(true);
}

void ProtoOptions::on_chkIgnoreSSLErrors_stateChanged(int)
{
	emit changed(true);
}