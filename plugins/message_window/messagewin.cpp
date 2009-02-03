#include "messagewin.h"
#include <QDateTime>

MessageWin::MessageWin(const QString &proto, const QString &account, const QString &contact, const QString &nik, QWidget *parent)
	: QDialog(parent), proto_name(proto), account_id(account), contact_id(contact), nick(nik)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);
	if(!nick.isEmpty())
		setWindowTitle(nick);
	else
		setWindowTitle(contact_id);
	ui.edMsgLog->setFont(QFont("tahoma", 12));
}

MessageWin::~MessageWin()
{

}

QString timestamp() {
	QDateTime dt = QDateTime::currentDateTime();
	return "<small>" + dt.toString("dd/MM/yy hh:mm: ") + "</small>";
}

void MessageWin::msgRecv(const QString &msg) {
	show();
	ui.edMsgLog->append("<font color='#000000'>" + timestamp() + "</font>" + "<font color='#ff00ff'>" + msg + "</font>");
	//activateWindow();
}

void MessageWin::on_btnSend_clicked() {
	ui.edMsgLog->append("<font color='#7f7f7f'>" + timestamp() + "</font>" + "<font color='#0000ff'>" + Qt::escape(ui.edMsg->text()) + "</font>");
	emit msgSend(proto_name, account_id, contact_id, ui.edMsg->text());
	ui.edMsg->clear();
	ui.edMsg->setFocus();
}
