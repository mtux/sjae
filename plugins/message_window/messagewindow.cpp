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

MessageWindow::MessageWindow(): next_msg_id(1) {
}

MessageWindow::~MessageWindow() {
}

bool MessageWindow::load(CoreI *core) {
	core_i = core;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((icons_i = (IconsI *)core_i->get_interface(INAME_ICONS)) == 0) return false;
	if((clist_i = (CListI *)core_i->get_interface(INAME_CLIST)) == 0) return false;

	return true;
}

bool MessageWindow::modules_loaded() {
	connect(accounts_i, SIGNAL(account_removed(const QString &, const QString &)), this, SLOT(account_removed(const QString &, const QString &)));

	QStringList proto_names = accounts_i->protocol_names();
	foreach(QString proto_name, proto_names) {
		ProtocolI *proto = accounts_i->get_proto_interface(proto_name);
		connect(proto, SIGNAL(message_recv(const QString &, const QString &, const QString &, const QString &)), this, SLOT(message_recv(const QString &, const QString &, const QString &, const QString &)));
		connect(proto, SIGNAL(status_change(const QString &, const QString &, const QString &, GlobalStatus)), this, SLOT(status_change(const QString &, const QString &, const QString &, GlobalStatus)));
		qDebug() << "message window connecting message_recv signal for protocol" << proto_name;
	}

	connect(clist_i, SIGNAL(contact_dbl_clicked(const QString &, const QString &, const QString &)), this, SLOT(open_window(const QString &, const QString &, const QString &)));
	return true;
}

bool MessageWindow::pre_shutdown() {
	QMapIterator<QString, QMap<QString, QMap<QString, SplitterWin *> > > i(windows);
	while(i.hasNext()) {
		i.next();
		QMapIterator<QString, QMap<QString, SplitterWin *> > j(i.value());
		while(j.hasNext()) {
			j.next();
			QMapIterator<QString, SplitterWin *> k(j.value());
			while(k.hasNext()) {
				k.next();
				delete k.value();
			}
		}
	}
	windows.clear();
	return true;
}

bool MessageWindow::unload() {
	return true;
}

const PluginInfo &MessageWindow::get_plugin_info() {
	return info;
}

/////////////////////////////
bool MessageWindow::window_exists(const QString &proto_name, const QString &account_id, const QString &contact_id) {
	return (windows.contains(proto_name) && windows[proto_name].contains(account_id) && windows[proto_name][account_id].contains(contact_id));
}

SplitterWin *MessageWindow::get_window(const QString &proto_name, const QString &account_id, const QString &contact_id) {
	if(!window_exists(proto_name, account_id, contact_id)) {
		SplitterWin *win = new SplitterWin(proto_name, account_id, contact_id);
		windows[proto_name][account_id][contact_id] = win;

		ProtocolI *proto = accounts_i->get_proto_interface(proto_name);
		win->setWindowIcon(icons_i->get_account_status_icon(proto, account_id, proto->get_contact_status(account_id, contact_id)));

		connect(win, SIGNAL(msgSend(const QString &, const QString &, const QString &, const QString &)), this, SLOT(message_send(const QString &, const QString &, const QString &, const QString &)));
		//win->setWindowIcon(i
	}

	return windows[proto_name][account_id][contact_id];
}

void MessageWindow::account_added(const QString &proto_name, const QString &id) {
}

void MessageWindow::account_removed(const QString &proto_name, const QString &id) {
}

void MessageWindow::message_recv(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg) {
	SplitterWin *win = get_window(proto_name, account_id, contact_id);
	win->msgRecv(msg);
}

void MessageWindow::status_change(const QString &proto_name, const QString &account_id, const QString &contact_id, GlobalStatus gs) {
	if(window_exists(proto_name, account_id, contact_id)) {
		SplitterWin *win = get_window(proto_name, account_id, contact_id);
		win->setWindowIcon(icons_i->get_account_status_icon(accounts_i->get_proto_interface(proto_name), account_id, gs));
	}
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
	SplitterWin *win = get_window(proto_name, account_id, contact_id);
	win->show();
	win->activateWindow();
}

/////////////////////////////

Q_EXPORT_PLUGIN2(messageWindow, MessageWindow)
