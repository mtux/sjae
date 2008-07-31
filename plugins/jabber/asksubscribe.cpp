#include "asksubscribe.h"

AskSubscribe::AskSubscribe(const QString &jid, QWidget *parent)
: QDialog(parent)
{
	ui.setupUi(this);
	ui.lblUser->setText(jid);
	setAttribute(Qt::WA_DeleteOnClose);

	connect(this, SIGNAL(accepted()), this, SLOT(emitGrant()));
}

AskSubscribe::~AskSubscribe()
{

}

void AskSubscribe::emitGrant() {
	emit grant(ui.lblUser->text());
}

