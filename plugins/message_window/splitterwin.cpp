#include "splitterwin.h"
#include <QDateTime>
#include <QSettings>

SplitterWin::SplitterWin(const QString &proto, const QString &account, const QString &contact, const QString &nik, QWidget *parent)
	: QSplitter(parent), proto_name(proto), account_id(account), contact_id(contact), nick(nik)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_QuitOnClose, false);

	if(!nick.isEmpty())
		setWindowTitle(nick);
	else
		setWindowTitle(contact_id);

	ui.edMsgLog->setFont(QFont("tahoma", 12));

	connect(ui.widget, SIGNAL(msgSend(const QString &)), this, SLOT(msgSend(const QString &)));

	QSettings settings;
	restoreGeometry(settings.value("MessageWindow/geometry/" + proto_name + ":" + account_id + ":" + contact_id).toByteArray());
}

SplitterWin::~SplitterWin() {
	QSettings settings;
	settings.setValue("MessageWindow/geometry/" + proto_name + ":" + account_id + ":" + contact_id, saveGeometry());
}

QString timestamp() {
	QDateTime dt = QDateTime::currentDateTime();
	return "<small>" + dt.toString("dd/MM/yy hh:mm: ") + "</small>";
}

void SplitterWin::msgRecv(const QString &msg) {
	QString dispMsg = Qt::escape(msg);
	dispMsg.replace("\n", "<br>");
	ui.edMsgLog->append("<font color='#000000'>" + timestamp() + "</font>" + "<font color='#ff00ff'>" + dispMsg + "</font>");
	
	show();
	activateWindow();
	raise();
}

void SplitterWin::msgSend(const QString &msg) {
	QString dispMsg = Qt::escape(msg);
	dispMsg.replace("\n", "<br>");
	ui.edMsgLog->append("<font color='#7f7f7f'>" + timestamp() + "</font>" + "<font color='#0000ff'>" + dispMsg + "</font>");
	
	emit msgSend(proto_name, account_id, contact_id, msg);
}

