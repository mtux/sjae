#include "messagewin.h"

MessageWin::MessageWin(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.edMsg, SIGNAL(returnPressed()), this, SLOT(on_btnSend_clicked()));
	ui.edMsg->setFocus();
}

MessageWin::~MessageWin()
{

}

bool MessageWin::okToSend(QString msg) {
	for(int i = 0; i < msg.length(); i++) {
		if(!msg.at(i).isSpace())
			return true;
	}
	return false;
}

void MessageWin::on_btnSend_clicked() {
	QString msg = ui.edMsg->document()->toPlainText();
	if(okToSend(msg)) {

		emit msgSend(msg);
		
		ui.edMsg->clear();
		ui.edMsg->setFocus();
	}
}
