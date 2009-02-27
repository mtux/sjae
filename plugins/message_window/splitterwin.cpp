#include "splitterwin.h"
#include <QDateTime>
#include <QSettings>
#include <QWebFrame>
#include <QDesktopServices>
#include <QDebug>

#define MAX_MESSAGES		500

#define LINK_PATTERN		"\\b(http://\\S+|\\S+\\.co(?:m)?(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?" \
	"|\\S+\\.org(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?|\\S+\\.net(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?" \
	"|\\S+\\.gov(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?|\\S+\\.biz(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?" \
	"|\\S+\\.info(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?|\\S+\\.travel(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?" \
	"|www\\.\\S+\\.\\S+)\\b"

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

void SplitterWin::hideEvent(QHideEvent *e) {
	gone();
	QSplitter::hideEvent(e);
}

void SplitterWin::showEvent(QShowEvent *e) {
	QSplitter::showEvent(e);
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
			events_i->fire_event(UserChatState(contact, chatState, this));
	}
}

void SplitterWin::update_title() {
	if(getNick() != contact->contact_id)
		setWindowTitle(getNick() + " (" + contact->contact_id + ")");
	else
		setWindowTitle(contact->contact_id);
}

void SplitterWin::openLink(const QUrl &url) {
	QUrl myUrl = url;
	if(myUrl.scheme().isEmpty())
		myUrl.setScheme("http");
	QDesktopServices::openUrl(myUrl);
}

QString SplitterWin::getContent() {
	QString ret;
	bool first = true, last_incomming;
	foreach(MessageData item, content) {
		if(first || item.incomming != last_incomming) {
			if(first)
				ret += QString("<div class='") + (item.incomming ? "incomming" : "outgoing") + "'>";
			else
				ret += QString("</div>\n<div class='") + (item.incomming ? "incomming" : "outgoing") + "'>";
		}
		ret += item.message;
		first = false;
		last_incomming = item.incomming;
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
	if(contact->properties.contains("handle")) return contact->properties["handle"].toString();
	if(contact->properties.contains("nick")) return contact->properties["nick"].toString();
	if(contact->properties.contains("name")) return contact->properties["name"].toString();
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

void SplitterWin::msgRecv(const QString &msg, QDateTime &time) {
	QString dispMsg = Qt::escape(msg);
	dispMsg.replace("\n", "<br />");
	dispMsg.replace(QRegExp(LINK_PATTERN), "<a href='http://\\1'>\\1</a>");
	
	QString text = "<div class='message'>";
	text += "<span class='info'>";
	text += timestamp(time);
	text += "<span class='nick'>" + Qt::escape(getNick()) + " </span>";
	text += "<span class='separator'>: </span></span>";
	text += "<span class='text'>" + dispMsg + "</span>";
	text += "</div>";

	content << MessageData(true, text);
	while(content.size() > MAX_MESSAGES)
		content.removeFirst();

	if(contactChatState != CS_ACTIVE)
		setContactChatState(CS_ACTIVE);
	else
		update_log();
	
	show();
	activateWindow();
	raise();

	active();
}

void SplitterWin::msgSend(const QString &msg) {
	QString dispMsg = Qt::escape(msg);
	dispMsg.replace("\n", "<br />");
	dispMsg.replace(QRegExp(LINK_PATTERN), "<a href='http://\\1'>\\1</a>");

	QString text = "<div class='message'>";
	text += "<span class='info'>";
	text += timestamp(QDateTime::currentDateTime());
	text += "<span class='nick'>" + Qt::escape(contact->account->nick) + " </span>";
	text += "<span class='separator'>: </span></span>";
	text += "<span class='text'>" + dispMsg + "</span>";
	text += "</div>";

	content << MessageData(false, text);
	while(content.size() > MAX_MESSAGES)
		content.removeFirst();
	update_log();
	
	MessageSend ms(msg, 0, contact, this);
	events_i->fire_event(ms);

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
