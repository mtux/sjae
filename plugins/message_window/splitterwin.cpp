#include "splitterwin.h"
#include <QDateTime>
#include <QSettings>
#include <QWebFrame>
#include <QDesktopServices>
#include <QDebug>

#define MAX_MESSAGES		100
#define LINK_PATTERN		"\\b(http://\\S+|\\S+\\.co(?:m)?(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?" \
	"|\\S+\\.org(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?|\\S+\\.net(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?" \
	"|\\S+\\.gov(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?|\\S+\\.biz(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?" \
	"|\\S+\\.info(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?|\\S+\\.travel(?:\\.[a-zA-Z]{2})?(?:[/\\?]\\S*)?" \
	"|www\\.\\S+\\.\\S+)\\b"

SplitterWin::SplitterWin(Contact *c, EventsI *ei, QWidget *parent)
	: QSplitter(parent), contact(c), events_i(ei),
		showDate(true), showTime(true), showNick(true)
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
}

SplitterWin::~SplitterWin() {
	QSettings settings;
	settings.setValue("MessageWindow/geometry/" + contact->account->proto->name() + ":" + contact->account->account_id + ":" + contact->contact_id, saveGeometry());
}

void SplitterWin::update_title() {
	if(getNick(contact) != contact->contact_id)
		setWindowTitle(getNick(contact) + " (" + contact->contact_id + ")");
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

	return ret;
}

void SplitterWin::update_log() {
	QString page = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";
	page += "<html xmlns='http://www.w3.org/1999/xhtml'>\n<head><title>Saje Message Log</title><style type='text/css'>" + style + "</style></head>\n<body>\n" + getContent() + "</body>\n</html>";
	qDebug() << "webview content:" << page;
	ui.edMsgLog->setContent(page.toUtf8(), "application/xhtml+xml");
	ui.edMsgLog->page()->mainFrame()->setScrollBarValue(Qt::Vertical, ui.edMsgLog->page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
}

QString SplitterWin::getNick(Contact *contact) {
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
	text += "<span class='nick'>" + Qt::escape(getNick(contact)) + " </span>";
	text += "<span class='separator'>: </span></span>";
	text += "<span class='text'>" + dispMsg + "</span>";
	text += "</div>";

	content << MessageData(true, text);
	while(content.size() > MAX_MESSAGES)
		content.removeFirst();

	update_log();
	
	show();
	activateWindow();
	raise();
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
}

