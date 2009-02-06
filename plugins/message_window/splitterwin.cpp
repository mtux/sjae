#include "splitterwin.h"
#include <QDateTime>
#include <QSettings>

SplitterWin::SplitterWin(const QString &proto, const QString &account, const QString &contact, const QString &cnik, const QString &my_nik, QWidget *parent)
	: QSplitter(parent), proto_name(proto), account_id(account), contact_id(contact), contact_nick(cnik), my_nick(my_nik),
	showDate(true), showTime(true), showNick(false)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_QuitOnClose, false);

	if(!contact_nick.isEmpty())
		setWindowTitle(contact_nick + " (" + contact_id + ")");
	else
		setWindowTitle(contact_id);

	//ui.edMsgLog->setFont(QFont("tahoma", 12));

	connect(ui.widget, SIGNAL(msgSend(const QString &)), this, SLOT(msgSend(const QString &)));

	QSettings settings;
	restoreGeometry(settings.value("MessageWindow/geometry/" + proto_name + ":" + account_id + ":" + contact_id).toByteArray());
}

SplitterWin::~SplitterWin() {
	QSettings settings;
	settings.setValue("MessageWindow/geometry/" + proto_name + ":" + account_id + ":" + contact_id, saveGeometry());
}

void SplitterWin::setLogStyleSheet(const QString &styleSheet) {
	ui.edMsgLog->document()->setDefaultStyleSheet(styleSheet);
	ui.edMsgLog->setHtml(ui.edMsgLog->document()->toHtml());
}

QString SplitterWin::timestamp() {
	QDateTime dt = QDateTime::currentDateTime();
	QString ret = "";
	if(showDate) ret += "<span class='date'>" + dt.toString("dd/MM/yy") + "</span>";
	if(showTime) {
		if(ret.size()) ret += " ";
		ret += "<span class='time'>" + dt.toString("hh:mm") + "</span>";
	}
	if(ret.size()) {
		ret.prepend("<span class='timestamp'>");
		ret.append("</span>");
	}
	return ret;
}

void SplitterWin::msgRecv(const QString &msg) {
	QString dispMsg = Qt::escape(msg);
	dispMsg.replace("\n", "<br>");
	QString ts = timestamp();
	QString text = ts;
	if(showNick) {
		if(text.size()) text += " ";
		text += "<span class='nick'>" + (contact_nick.isEmpty() ? contact_id : contact_nick) + "</span>";
	}
	if(text.size()) text += ": ";
	text += "<span class='message'>" + dispMsg + "</span>";
	text.prepend("<span class='incomming'>");
	text.append("</span>");
	ui.edMsgLog->append(text);
	
	show();
	activateWindow();
	raise();
}

void SplitterWin::msgSend(const QString &msg) {
	QString dispMsg = Qt::escape(msg);
	dispMsg.replace("\n", "<br>");
	QString ts = timestamp();
	QString text = ts;
	if(showNick) {
		if(text.size()) text += " ";
		text += "<span class='nick'>" + my_nick + "</span>";
	}
	if(text.size()) text += ": ";
	text += "<span class='message'>" + dispMsg + "</span>";
	text.prepend("<span class='outgoing'>");
	text.append("</span>");
	ui.edMsgLog->append(text);
	
	emit msgSend(proto_name, account_id, contact_id, msg);
}

