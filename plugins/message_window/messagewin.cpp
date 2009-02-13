#include "messagewin.h"

MessageWin::MessageWin(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.edMsg, SIGNAL(textChanged()), this, SIGNAL(textChanged()));
	connect(ui.edMsg, SIGNAL(returnPressed()), this, SLOT(on_btnSend_clicked()));

	ui.edMsg->setFocus();
}

MessageWin::~MessageWin()
{

}

bool MessageWin::okToSend() {
	QString msg = ui.edMsg->document()->toPlainText();
	for(int i = 0; i < msg.length(); i++) {
		if(!msg.at(i).isSpace())
			return true;
	}
	return false;
}

void MessageWin::on_btnSend_clicked() {
	if(okToSend()) {

		emit msgSend(ui.edMsg->document()->toPlainText());
		
		disconnect(ui.edMsg, SIGNAL(textChanged()), this, SIGNAL(textChanged()));
		ui.edMsg->clear();
		connect(ui.edMsg, SIGNAL(textChanged()), this, SIGNAL(textChanged()));

		ui.edMsg->setFocus();
	}
}
