#include "jabber.h"
#include <QtPlugin>
#include <QDebug>
#include <icons_i.h>
#include <main_window_i.h>

#include "jabbersearchwin.h"

PluginInfo info = {
	0x800,
	"Jabber Protocol",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Jabber Protocol",
	0x00000001
};


jabber::jabber(): proto(0), send_direct(0)
{

}

jabber::~jabber()
{

}

bool jabber::load(CoreI *core) {
	registerDiscoMetaTypes();

	core_i = core;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	IconsI *icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);
	if(icons_i) icons_i->add_icon("Proto/Jabber", QPixmap(":/icons/Resources/bulb.PNG"), "Protocols/Jabber");

	send_direct = new SendDirect();
	connect(this, SIGNAL(destroyed()), send_direct, SLOT(deleteLater()));

	connect(accounts_i, SIGNAL(account_changed(const QString &, const QString &)), this, SLOT(account_changed(const QString &, const QString &)));
	connect(accounts_i, SIGNAL(account_added(const QString &, const QString &)), this, SLOT(account_added(const QString &, const QString &)));
	connect(accounts_i, SIGNAL(account_removed(const QString &, const QString &)), this, SLOT(account_removed(const QString &, const QString &)));

	proto = new JabberProto(this);
	accounts_i->register_protocol(proto);

	connect(send_direct, SIGNAL(send_direct(const QString &, const QString &)), proto, SLOT(direct_send(const QString &, const QString &)));
	return true;
}

bool jabber::modules_loaded() {
	//OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	//if(options_i)
	//	options_i->add_page("User Interface/Contact List", new CListOptions(this));

	main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW);
	if(main_win_i) {
		menu = new QMenu(tr("Jabber"));

		QAction *act = menu->addAction("Send Direct to Server");
		connect(act, SIGNAL(triggered()), send_direct, SLOT(show()));

		main_win_i->manage_window_position(send_direct);

		main_win_i->add_submenu(menu);
	} else {
		send_direct->show();
	}

	proto->modules_loaded();

	return true;
}

void jabber::account_changed(const QString &proto_name, const QString &account_id) {
	if(proto_name == "Jabber") {
		proto->account_changed(account_id);
	}
}

void jabber::account_added(const QString &proto_name, const QString &account_id) {
	if(proto_name == "Jabber") {
		proto->account_added(account_id);
		if(send_direct) send_direct->add_account(account_id);
	}
}

void jabber::account_removed(const QString &proto_name, const QString &account_id) {
	if(proto_name == "Jabber") {
		proto->account_removed(account_id);
		if(send_direct) send_direct->remove_account(account_id);
	}
}

bool jabber::pre_shutdown() {
	// disconnects sets all contacts offline (changes icon, proto must remain registered)
	proto->deleteContexts(); 
	return true;
}

bool jabber::unload() {
	accounts_i->deregister_protocol(proto);
	delete proto;
	return true;
}

const PluginInfo &jabber::get_plugin_info() {
	return info;
}

///////////////////////////////////////////
JabberProto::JabberProto(jabber *jabberPlugin): plugin(jabberPlugin) {
	service_discovery = new ServiceDiscovery();
	connect(this, SIGNAL(destroyed()), service_discovery, SLOT(deleteLater()));
	gateways = new GatewayList();
	connect(this, SIGNAL(destroyed()), gateways, SLOT(deleteLater()));
	connect(gateways, SIGNAL(gateway_register(const QString &, const QString &)), this, SLOT(gateway_register(const QString &, const QString &)));
	connect(gateways, SIGNAL(gateway_unregister(const QString &, const QString &)), this, SLOT(gateway_unregister(const QString &, const QString &)));
}

void JabberProto::modules_loaded() {
	if(plugin->main_win_i) {
		QAction *act = plugin->menu->addAction(tr("Service Discovery"));
		connect(act, SIGNAL(triggered()), service_discovery, SLOT(show()));
		plugin->main_win_i->manage_window_position(service_discovery);

		act = plugin->menu->addAction(tr("Gateways"));
		connect(act, SIGNAL(triggered()), gateways, SLOT(show()));
		plugin->main_win_i->manage_window_position(gateways);
	} else {
		service_discovery->show();
		gateways->show();
	}
}

JabberProto::~JabberProto() {
	deleteContexts();
}

void JabberProto::deleteContexts() {
	QMapIterator<QString, JabberCtx *> i(ctx);
	while(i.hasNext()) {
		i.next();
		delete i.value();
	}
	ctx.clear();
}

const GlobalStatus JabberProto::closest_status_to(GlobalStatus gs) const {
	switch(gs) {
		case ST_OFFLINE: 
		case ST_ONLINE: 
		case ST_INVISIBLE: 
		case ST_FREETOCHAT: 
		case ST_SHORTAWAY: 
		case ST_LONGAWAY:
		case ST_DND:
		case ST_CONNECTING:
			return gs;
		case ST_OUTTOLUNCH:
		case ST_ONTHEPHONE:
			return ST_SHORTAWAY;

	}
	return ST_OFFLINE;
}

bool JabberProto::message_send(const QString &account_id, const QString &contact_id, const QString &msg, int id) {
	if(ctx.contains(account_id)) {
		ctx[account_id]->msgSend(contact_id, msg, id);
	}
	return false;
}

const QList<GlobalStatus> JabberProto::statuses() const {
	return QList<GlobalStatus>() 
		<< ST_OFFLINE
		<< ST_ONLINE
		<< ST_INVISIBLE
		<< ST_FREETOCHAT
		<< ST_SHORTAWAY
		<< ST_LONGAWAY
		<< ST_DND;
}


const GlobalStatus JabberProto::get_status(const QString &account_id) const {
	if(ctx.contains(account_id)) return ctx[account_id]->getCurrentStatus();
	return ST_OFFLINE;
}

bool JabberProto::set_status(const QString &account_id, GlobalStatus gs) {
	qDebug() << "set status called," << hr_status_name[gs];

	if(!ctx.contains(account_id)) {
		ctx[account_id] = new JabberCtx(account_id, plugin->accounts_i->account_info(name(), account_id), plugin->core_i, this);
		connect(ctx[account_id], SIGNAL(statusChanged(const QString &, GlobalStatus)), this, SLOT(context_status_change(const QString &, GlobalStatus)));
		connect(ctx[account_id], SIGNAL(msgRecv(const QString &, const QString &, const QString &)), this, SLOT(context_message_recv(const QString &, const QString &, const QString &)));
		connect(ctx[account_id], SIGNAL(msgAck(int)), this, SIGNAL(msgAck(int)));

		if(service_discovery) {
			connect(ctx[account_id], SIGNAL(gotDiscoInfo(const DiscoInfo &)), service_discovery, SLOT(gotDiscoInfo(const DiscoInfo &)));
			connect(ctx[account_id], SIGNAL(gotDiscoItems(const DiscoItems &)), service_discovery, SLOT(gotDiscoItems(const DiscoItems &)));

			connect(service_discovery, SIGNAL(queryInfo(const QString &, const QString &)), ctx[account_id], SLOT(sendIqQueryDiscoInfo(const QString &, const QString &)));
			connect(service_discovery, SIGNAL(queryItems(const QString &, const QString &)), ctx[account_id], SLOT(sendIqQueryDiscoItems(const QString &, const QString &)));

		}
		if(gateways) {
			connect(ctx[account_id], SIGNAL(gotGateway(const QString &, const QString &)), gateways, SLOT(add_gateway(const QString &, const QString &)));
		}
	}

	ctx[account_id]->requestStatus(gs);

	return true;
}

void JabberProto::add_contact(const QString &account_id, const QString &contact_id) {
	if(ctx.contains(account_id)) {
		ctx[account_id]->addContact(contact_id);
	} else
		qWarning() << "add_contact: no account called" << account_id;

}

bool JabberProto::direct_send(const QString &account_id, const QString &text) {
	if(ctx.contains(account_id)) {
		return ctx[account_id]->directSend(text);
	} else {
		qWarning() << "direct_send: no account called" << account_id;
		return false;
	}
}

bool JabberProto::gateway_register(const QString &account_id, const QString &gateway) {
	if(ctx.contains(account_id)) {
		return ctx[account_id]->gatewayRegister(gateway);
	} else {
		qWarning() << "gateway_register: no account called" << account_id;
		return false;
	}
}

bool JabberProto::gateway_unregister(const QString &account_id, const QString &gateway) {
	if(ctx.contains(account_id)) {
		return ctx[account_id]->gatewayUnregister(gateway);
	} else {
		qWarning() << "gateway_unregister: no account called" << account_id;
		return false;
	}
}

void JabberProto::context_status_change(const QString &account_id, GlobalStatus gs) {
	emit local_status_change(name(), account_id, gs);
	if(gs == ST_OFFLINE) {
		if(gateways) gateways->remove_account(account_id);
		//if(service_discovery)  ???
	}
}

void JabberProto::context_message_recv(const QString &account_id, const QString &contact_id, const QString &msg) {
	emit message_recv(name(), account_id, contact_id, msg);
}

bool JabberProto::remove_account_data(const QString &account_id) {
	if(ctx.contains(account_id)) {
		delete ctx[account_id];
		ctx.remove(account_id);
		return true;
	}
	if(gateways)
		gateways->remove_account(account_id);

	return false;
}

bool JabberProto::update_account_data(const QString &account_id, const AccountInfo &info) {
	if(ctx.contains(account_id)) {
		ctx[account_id]->setAccountInfo(info);
	} else {
		ctx[account_id] = new JabberCtx(account_id, info, plugin->core_i, this);
		connect(ctx[account_id], SIGNAL(statusChanged(const QString &, GlobalStatus)), this, SLOT(context_status_change(const QString &, GlobalStatus)));
		connect(ctx[account_id], SIGNAL(msgRecv(const QString &, const QString &, const QString &)), this, SLOT(context_message_recv(const QString &, const QString &, const QString &)));
		connect(ctx[account_id], SIGNAL(msgAck(int)), this, SIGNAL(msgAck(int)));

		if(service_discovery) {
			connect(ctx[account_id], SIGNAL(gotDiscoInfo(const DiscoInfo &)), service_discovery, SLOT(gotDiscoInfo(const DiscoInfo &)));
			connect(ctx[account_id], SIGNAL(gotDiscoItems(const DiscoItems &)), service_discovery, SLOT(gotDiscoItems(const DiscoItems &)));

			connect(service_discovery, SIGNAL(queryInfo(const QString &, const QString &)), ctx[account_id], SLOT(sendIqQueryDiscoInfo(const QString &, const QString &)));
			connect(service_discovery, SIGNAL(queryItems(const QString &, const QString &)), ctx[account_id], SLOT(sendIqQueryDiscoItems(const QString &, const QString &)));
		}
		if(gateways) {
			connect(ctx[account_id], SIGNAL(gotGateway(const QString &, const QString &)), gateways, SLOT(add_gateway(const QString &, const QString &)));
		}
	}

	return true;
}

bool JabberProto::get_account_data(const QString &account_id, AccountInfo &account_info) {
	if(ctx.contains(account_id)) {
		account_info = ctx[account_id]->get_account_info();
		return true;
	}
	return false;
}

void JabberProto::account_added(const QString &account_id) {
	/*
	if(!ctx.contains(account_id)) {
		ctx[account_id] = new JabberCtx(account_id, plugin->accounts_i->account_info(name(), account_id), plugin->core_i, this);
		connect(ctx[account_id], SIGNAL(statusChanged(const QString &, GlobalStatus)), this, SLOT(context_status_change(const QString &, GlobalStatus)));
		connect(ctx[account_id], SIGNAL(msgRecv(const QString &, const QString &, const QString &)), this, SLOT(context_message_recv(const QString &, const QString &, const QString &)));
	}
	*/
}

void JabberProto::account_removed(const QString &account_id) {
}

void JabberProto::account_changed(const QString &account_id) {
}

void JabberProto::parse_extra_data(QDomElement &node, const QString &account_id) {
	if(ctx.contains(account_id)) {
		ctx[account_id]->setUseSSL(node.attribute("useSSL", "false") == "true");
		ctx[account_id]->setConnectionHost(node.attribute("connectionHost", ""));
	}
}

void JabberProto::set_extra_data(QDomElement &node, const QString &account_id) {
	if(ctx.contains(account_id)) {
		node.setAttribute("useSSL", ctx[account_id]->getUseSSL() ? "true" : "false");
		if(!ctx[account_id]->getConnectionHost().isEmpty())
			node.setAttribute("connectionHost", ctx[account_id]->getConnectionHost());
	}
}

AccountExtra *JabberProto::create_account_extra(const QString &account_id) {
	ProtoOptions *po = new ProtoOptions();
	if(ctx.contains(account_id))
		po->setContext(ctx[account_id]);
	return po;
}

ProtoSearchWindowI *JabberProto::create_search_window() {
	return new JabberSearchWin(this);
}

///////////////////////////////////////////

Q_EXPORT_PLUGIN2(jabberPlugin, jabber)
