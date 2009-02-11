#include "splitterwin.h"
#include <QDateTime>
#include <QSettings>

SplitterWin::SplitterWin(Contact *c, EventsI *ei, QWidget *parent)
	: QSplitter(parent), contact(c), events_i(ei),
		showDate(true), showTime(true), showNick(false)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_QuitOnClose, false);

	if(!contact->properties.contains("nick"))
		setWindowTitle(contact->properties["nick"].toString() + " (" + contact->contact_id + ")");
	else
		setWindowTitle(contact->contact_id);

	//ui.edMsgLog->setFont(QFont("tahoma", 12));

	connect(ui.widget, SIGNAL(msgSend(const QString &)), this, SLOT(msgSend(const QString &)));

	QSettings settings;
	restoreGeometry(settings.value("MessageWindow/geometry/" + contact->account->proto->name() + ":" + contact->account->account_id + ":" + contact->contact_id).toByteArray());
}

SplitterWin::~SplitterWin() {
	QSettings settings;
	settings.setValue("MessageWindow/geometry/" + contact->account->proto->name() + ":" + contact->account->account_id + ":" + contact->contact_id, saveGeometry());
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
		text += "<span class='nick'>" + (contact->properties.contains("nick") ? contact->contact_id : contact->properties["nick"].toString()) + "</span>";
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
		text += "<span class='nick'>" + contact->account->nick + "</span>";
	}
	if(text.size()) text += ": ";
	text += "<span class='message'>" + dispMsg + "</span>";
	text.prepend("<span class='outgoing'>");
	text.append("</span>");
	ui.edMsgLog->append(text);
	
	MessageSend ms(msg, 0, contact, this);
	events_i->fire_event(ms);
}

