#include "protooptions.h"

ProtoOptions::ProtoOptions(QWidget *parent)
	: AccountExtra(parent), ctx(0)
{
	ui.setupUi(this);
}

ProtoOptions::~ProtoOptions() {

}

void ProtoOptions::set_account_info(const AccountInfo &info) {
}

void ProtoOptions::setContext(JabberCtx *c) {
	ctx = c;
	reset();
}

bool ProtoOptions::apply() {
	if(ctx) {
		ctx->setUseSSL(ui.chkSSL->isChecked());
		ctx->setConnectionHost(ui.edConHost->text());
		return true;
	}
	return false;
}

void ProtoOptions::reset() {
	if(ctx) {
		ui.chkSSL->setChecked(ctx->getUseSSL());
		ui.edConHost->setText(ctx->getConnectionHost());
	}
}

void ProtoOptions::on_chkSSL_stateChanged(int) {
	emit changed(true);
}


void ProtoOptions::on_edConHost_textChanged(const QString &) {
	emit changed(true);

}