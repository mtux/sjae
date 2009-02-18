#include "splitterwin.h"
#include <QDateTime>
#include <QSettings>
#include <QWebFrame>
#include <QDesktopServices>
#include <QDebug>

#define MAX_MESSAGES		500

#define RX_DOMAIN		"(?:\\.co(?:m)?|\\.org|\\.net|\\.gov|\\.biz|\\.info|\\.travel)(?:\\.[a-z]{2})?"
#define RX_PROTOS		"(?:http(?:s)?://|ftp://|mailto:|file://)?"
#define RX_PORT			"(?:\\:\\d{0,5})?"
#define RX_EMAIL		"\\w+@\\w+(?:\\.\\w+)*" RX_DOMAIN
#define RX_OTHER		"\\w+(?:\\.\\w+)*" RX_DOMAIN RX_PORT "(?:[/\\?]\\S*)?"
#define LP				"\\b(" RX_PROTOS ")(" RX_EMAIL "|" RX_OTHER ")\\b"

SplitterWin::SplitterWin(Contact *c, EventsI *ei, QWidget *parent)
	: QSplitter(parent), contact(c), events_i(ei),
		showDate(true), showTime(true), showNick(true), sendChatState(true), contactChatState(CS_INACTIVE), chatState(CS_INACTIVE)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_QuitOnClose, false);

	update_title();

	//ui.edMsgLog->setFont(QFont("tahoma", 12));

	connect(ui.widget, SIGNAL(msgSend(const QString &)), this, SLOT(msgSend(const QString &)));

	QSettings settings;
	restoreGeometry(settings.value("MessageWindow/geometry/" + contact->account->proto->name() + ":" + contact->account->account_id + ":" + contact->contact_id).toByteArray());

	ui.edMsgLog->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	connect(ui.edMsgLog, SIGNAL(linkClicked(const QUrl &)), this, SLOT(openLink(const QUrl &)));

	connect(&pauseTimer, SIGNAL(timeout()), this, SLOT(paused()));
	connect(&inactiveTimer, SIGNAL(timeout()), this, SLOT(inactive()));

	pauseTimer.setInterval(5000);
	inactiveTimer.setInterval(30000);
	
	connect(ui.widget, SIGNAL(textChanged()), this, SLOT(composing()));

	update_log();
}

SplitterWin::~SplitterWin() {
	QSettings settings;
	settings.setValue("MessageWindow/geometry/" + contact->account->proto->name() + ":" + contact->account->account_id + ":" + contact->contact_id, saveGeometry());
}

void SplitterWin::addEvents(QList<Message> &events) {
	foreach(Message m, events) {
		addToLog(m);
	}
	update_log();
}

void SplitterWin::hideEvent(QHideEvent *e) {
	gone();
	QSplitter::hideEvent(e);
}

void SplitterWin::showEvent(QShowEvent *e) {
	QSplitter::showEvent(e);
}

void SplitterWin::closeEvent(QCloseEvent *e) {
	QSplitter::closeEvent(e);
	emit closed(contact);
}

void SplitterWin::setSendChatState(bool f) {
	sendChatState = f;
}

void SplitterWin::setContactChatState(ChatStateType state) {
	if(state != contactChatState) {
		contactChatState = state;
		update_log();
	}
}

void SplitterWin::setUserChatState(ChatStateType state) {
	if(state != chatState) {
		chatState = state;
		if(sendChatState)
			events_i->fire_event(ChatState(contact, chatState, false, this));
	}
}

void SplitterWin::update_title() {
	if(getNick() != contact->contact_id)
		setWindowTitle(getNick() + " (" + contact->contact_id + ")");
	else
		setWindowTitle(contact->contact_id);
}

void SplitterWin::openLink(const QUrl &url) {
	QDesktopServices::openUrl(url);
}

QString SplitterWin::format_text(Message &m) {
	QString nick = (m.type == EventsI::ET_INCOMMING ? Qt::escape(getNick()) : Qt::escape(m.contact->account->nick));
	QString msg = m.text;
	if(msg.startsWith("/me "))
		msg.replace(0, 4, "* " + nick + " ");
	QString dispMsg = Qt::escape(msg);
	dispMsg.replace("\n", "<br />\n");
	linkUrls(dispMsg);
	
	QString text = "<div class='message'>";
	text += "<span class='info'>";
	text += timestamp(m.timestamp);
	text += "<span class='nick'>" + nick + " </span>";
	text += "<span class='separator'>: </span></span>";
	text += "<span class='text'>" + dispMsg + "</span>";
	text += "</div>";

	return text;
}

QString SplitterWin::getContent() {
	QString ret;
	bool first = true, incomming, last_incomming;
	foreach(Message item, content) {
		incomming = (item.type == EventsI::ET_INCOMMING);
		if(first || incomming != last_incomming) {
			if(first)
				ret += QString("<div class='") + (incomming ? "incomming" : "outgoing") + "'>";
			else
				ret += QString("</div>\n<div class='") + (incomming ? "incomming" : "outgoing") + "'>";
		}
		ret += format_text(item);
		first = false;
		last_incomming = incomming;
	}

	if(!first) ret += "</div>\n";

	switch(contactChatState) {
		case CS_INACTIVE:
			ret += "<div class='chat_state' id='inactive'><span class='nick'>" + getNick() + "</span><span class='state_text'> is inactive</span></div>";
			break;
		case CS_ACTIVE:
			ret += "<div class='chat_state' id='active'><span class='nick'>" + getNick() + "</span><span class='state_text'> is active</span></div>";
			break;
		case CS_COMPOSING:
			ret += "<div class='chat_state' id='composing'><span class='nick'>" + getNick() + "</span><span class='state_text'> is typing</span></div>";
			break;
		case CS_PAUSED:
			ret += "<div class='chat_state' id='paused'><span class='nick'>" + getNick() + "</span><span class='state_text'> has entered text</span></div>";
			break;
		case CS_GONE:
			ret += "<div class='chat_state' id='gone'><span class='nick'>" + getNick() + "</span><span class='state_text'> is gone</span></div>";
			break;
	}

	return ret;
}

void SplitterWin::update_log() {
	QString page = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";
	page += "<html xmlns='http://www.w3.org/1999/xhtml'>\n<head><title>Saje Message Log</title><style type='text/css'>" + style + "</style></head>\n<body>\n" + getContent() + "</body>\n</html>";
	ui.edMsgLog->setContent(page.toUtf8(), "application/xhtml+xml");
	ui.edMsgLog->page()->mainFrame()->setScrollBarValue(Qt::Vertical, ui.edMsgLog->page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
}

QString SplitterWin::getNick() {
	if(contact->has_property("handle")) return contact->get_property("handle").toString();
	if(contact->has_property("nick")) return contact->get_property("nick").toString();
	if(contact->has_property("name")) return contact->get_property("name").toString();
	return contact->contact_id;
}

void SplitterWin::setLogStyleSheet(const QString &styleSheet) {
	//ui.edMsgLog->document()->setDefaultStyleSheet(styleSheet);
	//ui.edMsgLog->setHtml(ui.edMsgLog->toHtml());
	style = styleSheet;
	update_log();
}

QString SplitterWin::timestamp(QDateTime &dt) {
	QString dateClass = "date";
	if(dt.date() == QDate::currentDate())
		dateClass = "date_today";

	QString ret = "<span class='timestamp'>";
	ret += "<span class='" + dateClass + "'>" + Qt::escape(dt.date().toString(Qt::SystemLocaleShortDate)) + " </span>";
	ret += "<span class='time'>" + Qt::escape(dt.time().toString(Qt::SystemLocaleShortDate)) + "</span>";
	ret += " </span>";

	return ret;
}

void SplitterWin::linkUrls(QString &str) {
	//dispMsg.replace(QRegExp(LP), "<a href='http://\\2'>\\1</a>");

	QRegExp rx(LP), rx_email("^" RX_EMAIL);
	int pos = 0, len;
	QString scheme, after;
	bool valid;
	while ((pos = rx.indexIn(str, pos)) != -1) {
		len = rx.matchedLength();

		//rx.cap(0) is whole match, rx.cap(1) is url scheme, rx.cap(2) is the rest
		
		scheme = rx.cap(1);
		valid = true;
		if(scheme.isEmpty()) {
			if(rx_email.indexIn(rx.cap(2)) != -1)
				scheme = "mailto:";
			else
				scheme = "http://";
		} else 
		if((scheme == "mailto:" && rx_email.indexIn(rx.cap(2)) == -1)
			|| (scheme != "mailto:" && rx_email.indexIn(rx.cap(2)) != -1)) 
		{
			valid = false;
		}
		if(valid) {
			after = "<a href='" + scheme + rx.cap(2) + "'>" + rx.cap(0) + "</a>";
			str.replace(pos, len, after);
			len = after.length();
		}

		pos += len;
	}
}

void SplitterWin::addToLog(Message &m) {
	content << m;
	while(content.size() > MAX_MESSAGES)
		content.removeFirst();
}

void SplitterWin::msgRecv(Message &m) {
	addToLog(m);

	if(contactChatState != CS_ACTIVE)
		setContactChatState(CS_ACTIVE);
	else
		update_log();
	
	active();
}

void SplitterWin::msgSend(const QString &msg) {
	Message m(contact, msg, false, 0, this);
	addToLog(m);

	update_log();
	
	events_i->fire_event(m);

	active();
}

////////////////////////
void SplitterWin::active() {
	inactiveTimer.stop();
	pauseTimer.stop();

	setUserChatState(CS_ACTIVE);
	
	inactiveTimer.start();
}

void SplitterWin::composing() {
	if(ui.widget->okToSend()) {
		pauseTimer.stop();
		inactiveTimer.stop();

		setUserChatState(CS_COMPOSING);

		pauseTimer.start();
		inactiveTimer.start();
	} else
		active();
}

void SplitterWin::paused() {
	setUserChatState(CS_PAUSED);
}

void SplitterWin::inactive() {
	setUserChatState(CS_INACTIVE);
}

void SplitterWin::gone() {
	pauseTimer.stop();
	inactiveTimer.stop();

	setUserChatState(CS_GONE);
}
