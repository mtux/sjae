#include "messagewindow.h"
#include <global_status.h>
#include <QtPlugin>
#include <QDebug>

PluginInfo info = {
	0x480,
	"Message Window",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Message Window",
	0x00000001
};

MessageWindow::MessageWindow(): next_msg_id(1)
{

}

MessageWindow::~MessageWindow()
{

}

bool MessageWindow::load(CoreI *core) {
	core_i = core;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((clist_i = (CListI *)core_i->get_interface(INAME_CLIST)) == 0) return false;

	return true;
}

bool MessageWindow::modules_loaded() {
	connect(accounts_i, SIGNAL(account_removed(const QString &, const QString &)), this, SLOT(account_removed(const QString &, const QString &)));

	QStringList proto_names = accounts_i->protocol_names();
	foreach(QString proto_name, proto_names) {
		connect(accounts_i->get_proto_interface(proto_name), SIGNAL(message_recv(const QString &, const QString &, const QString &, const QString &)), this, SLOT(message_recv(const QString &, const QString &, const QString &, const QString &)));
		qDebug() << "message window connecting message_recv signal for protocol" << proto_name;
	}

	connect(clist_i, SIGNAL(contact_dbl_clicked(const QString &, const QString &, const QString &)), this, SLOT(open_window(const QString &, const QString &, const QString &)));
	return true;
}

bool MessageWindow::pre_shutdown() {
	return true;
}

bool MessageWindow::unload() {
	return true;
}

const PluginInfo &MessageWindow::get_plugin_info() {
	return info;
}

/////////////////////////////

MessageWin *MessageWindow::get_window(const QString &proto_name, const QString &account_id, const QString &contact_id) {
	if(!windows.contains(proto_name) || !windows[proto_name].contains(account_id) || !windows[proto_name][account_id].contains(contact_id)) {
		MessageWin *win = new MessageWin(proto_name, account_id, contact_id);
		windows[proto_name][account_id][contact_id] = win;

		connect(win, SIGNAL(msgSend(const QString &, const QString &, const QString &, const QString &)), this, SLOT(message_send(const QString &, const QString &, const QString &, const QString &)));
	}

	return windows[proto_name][account_id][contact_id];
}

void MessageWindow::account_added(const QString &proto_name, const QString &id) {
}

void MessageWindow::account_removed(const QString &proto_name, const QString &id) {
}

void MessageWindow::message_recv(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg) {
	MessageWin *win = get_window(proto_name, account_id, contact_id);
	win->msgRecv(msg);
}

void MessageWindow::message_send(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg) {
	qDebug() << "message_send - proto:" << proto_name << "account_id:" << account_id << "contact_id:" << contact_id << "msg:" << msg;
	ProtocolI *proto = accounts_i->get_proto_interface(proto_name);
	if(proto)
		proto->message_send(account_id, contact_id, msg, next_msg_id++);
	else
		qDebug() << "no proto interface";
}

void MessageWindow::open_window(const QString &proto_name, const QString &account_id, const QString &contact_id) {
	MessageWin *win = get_window(proto_name, account_id, contact_id);
	win->show();
	win->activateWindow();
}

/////////////////////////////

Q_EXPORT_PLUGIN2(messageWindow, MessageWindow)
