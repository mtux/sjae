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


jabber::jabber(): proto(0)
{

}

jabber::~jabber()
{

}

bool jabber::load(CoreI *core) {
	registerDiscoMetaTypes();

	core_i = core;
	// interfaces required for contexts and protocol object
	if(core_i->get_interface(INAME_EVENTS) == 0) {
		qDebug() << "Jabber load failed: no events interface";
		return false;
	}
	if(core_i->get_interface(INAME_CONTACTINFO) == 0) {
		qDebug() << "Jabber load failed: no contact info interface";
		return false;
	}
	if(core_i->get_interface(INAME_ACCOUNTS) == 0) {
		qDebug() << "Jabber load failed: no accounts interface";
		return false;
	}

	IconsI *icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);
	if(icons_i) icons_i->add_icon("Proto/Jabber", QPixmap(":/icons/Resources/bulb.PNG"), "Protocols/Jabber");

	proto = new JabberProto(core_i);

	return true;
}

bool jabber::modules_loaded() {
	proto->modules_loaded();

	return true;
}

bool jabber::pre_shutdown() {
	// disconnects and sets all contacts offline (changes icon, proto must remain registered)
	proto->deleteContexts(); 
	return true;
}

bool jabber::unload() {
	delete proto;
	return true;
}

const PluginInfo &jabber::get_plugin_info() {
	return info;
}

///////////////////////////////////////////
JabberProto::JabberProto(CoreI *core) {
	core_i = core;
	accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS);
	events_i = (EventsI *)core_i->get_interface(INAME_EVENTS);

	accounts_i->register_protocol(this);

	events_i->add_event_listener(this, UUID_MSG_SEND);
	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->add_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->add_event_listener(this, UUID_ACCOUNT_STATUS_REQ);
	events_i->add_event_listener(this, UUID_CHAT_STATE, EVENT_TYPE_MASK_OUTGOING);

	service_discovery = new ServiceDiscovery();
	connect(this, SIGNAL(destroyed()), service_discovery, SLOT(deleteLater()));
	connect(service_discovery, SIGNAL(queryInfo(const QString &, const QString &, const QString &)), this, SLOT(contextQueryDiscoInfo(const QString &, const QString &, const QString &)));
	gateways = new GatewayList();
	connect(this, SIGNAL(destroyed()), gateways, SLOT(deleteLater()));
	connect(gateways, SIGNAL(gateway_register(const QString &, const QString &)), this, SLOT(gateway_register(const QString &, const QString &)));
	connect(gateways, SIGNAL(gateway_unregister(const QString &, const QString &)), this, SLOT(gateway_unregister(const QString &, const QString &)));
	ask_subscribe = new AskSubscribe();
	connect(this, SIGNAL(destroyed()), ask_subscribe, SLOT(deleteLater()));
	connect(ask_subscribe, SIGNAL(grant(const QString &, const QString &)), this, SLOT(handleGranted(const QString &, const QString &)));
	connect(ask_subscribe, SIGNAL(deny(const QString &, const QString &)), this, SLOT(handleDenied(const QString &, const QString &)));
	send_direct = new SendDirect();
	connect(this, SIGNAL(destroyed()), send_direct, SLOT(deleteLater()));
	connect(send_direct, SIGNAL(send_direct(const QString &, const QString &)), this, SLOT(direct_send(const QString &, const QString &)));
}

void JabberProto::modules_loaded() {
	MenusI *menus_i = (MenusI *)core_i->get_interface(INAME_MENUS);
	MainWindowI *main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW);
	if(menus_i) {
		QAction *act = menus_i->add_menu_action("Main Menu/Jabber", tr("Service Discovery"));
		connect(act, SIGNAL(triggered()), service_discovery, SLOT(show()));
		if(main_win_i) main_win_i->manage_window_position(service_discovery);

		act = menus_i->add_menu_action("Main Menu/Jabber", tr("Gateways"));
		connect(act, SIGNAL(triggered()), gateways, SLOT(show()));
		if(main_win_i) main_win_i->manage_window_position(gateways);

		act = menus_i->add_menu_action("Main Menu/Jabber", tr("Send Direct"));
		connect(act, SIGNAL(triggered()), send_direct, SLOT(show()));
		if(main_win_i) main_win_i->manage_window_position(send_direct);
	} else {
		service_discovery->show();
		gateways->show();
		send_direct->show();
	}
}

JabberProto::~JabberProto() {
	events_i->remove_event_listener(this, UUID_MSG);
	events_i->remove_event_listener(this, UUID_ACCOUNT_CHANGED);
	events_i->remove_event_listener(this, UUID_ACCOUNT_STATUS_REQ);
	events_i->remove_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->remove_event_listener(this, UUID_CHAT_STATE);

	accounts_i->deregister_protocol(this);

	deleteContexts();
}

void JabberProto::handleGranted(const QString &contact_id, const QString &account_id) {
	Account *acc = accounts_i->account_info(this, account_id);
	if(ctx.contains(acc))
		ctx[acc]->sendGrant(contact_id);
}

void JabberProto::handleDenied(const QString &contact_id, const QString &account_id) {
	Account *acc = accounts_i->account_info(this, account_id);
	if(ctx.contains(acc))
		ctx[acc]->sendRevoke(contact_id);
}

void JabberProto::deleteContexts() {
	QMapIterator<Account *, JabberCtx *> i(ctx);
	while(i.hasNext()) {
		i.next();
		delete i.value();
	}
	ctx.clear();
}

void JabberProto::contextQueryDiscoInfo(const QString &account_id, const QString &entity_jid, const QString &node) {
	Account *acc = accounts_i->account_info(this, account_id);
	if(ctx.contains(acc))
		ctx[acc]->sendIqQueryDiscoInfo(entity_jid, node);
	else
		qDebug() << "contextQueryDiscoInfo: no such account id -" << account_id;
}

void JabberProto::connect_context(JabberCtx *context) {
	if(service_discovery) {
		connect(context, SIGNAL(gotDiscoInfo(const DiscoInfo &)), service_discovery, SLOT(gotDiscoInfo(const DiscoInfo &)));
		connect(context, SIGNAL(gotDiscoItems(const DiscoItems &)), service_discovery, SLOT(gotDiscoItems(const DiscoItems &)));
	}
	if(gateways) {
		connect(context, SIGNAL(gotGateway(const QString &, const QString &)), gateways, SLOT(add_gateway(const QString &, const QString &)));
	}
	if(ask_subscribe) {
		connect(context, SIGNAL(grantRequested(const QString &, const QString &)), ask_subscribe, SLOT(addUser(const QString &, const QString &)));
	}
}

GlobalStatus JabberProto::closest_status_to(GlobalStatus gs) const {
	switch(gs) {
		case ST_OFFLINE: 
		case ST_ONLINE: 
		case ST_SHORTAWAY: 
		case ST_LONGAWAY:
		case ST_DND:
		case ST_CONNECTING:
			return gs;
		case ST_OUTTOLUNCH:
		case ST_ONTHEPHONE:
			return ST_SHORTAWAY;
		case ST_INVISIBLE: 
		case ST_FREETOCHAT: 
			return ST_ONLINE;
	}
	return ST_OFFLINE;
}

bool JabberProto::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_MSG_SEND) {
		MessageSendReq &m = static_cast<MessageSendReq &>(e);
		if(ctx.contains(m.contact->account))
			ctx[m.contact->account]->msgSend(m.contact, m.text, m.id);
	} else if(e.uuid == UUID_ACCOUNT_STATUS_REQ) {
		AccountStatusReq &as = static_cast<AccountStatusReq &>(e);
		if(ctx.contains(as.account))
			ctx[as.account]->requestStatus(closest_status_to(as.status));
	} else if(e.uuid == UUID_ACCOUNT_CHANGED) {
		AccountChanged &ac = static_cast<AccountChanged &>(e);
		if(ac.removed) remove_account_data(ac.account);
		else update_account_data(ac.account);
	} else if(e.uuid == UUID_CHAT_STATE) {
		ChatState &cs = static_cast<ChatState &>(e);
		if(ctx.contains(cs.contact->account))
			ctx[cs.contact->account]->setUserChatState(cs.contact, cs.state_type);
	} else if(e.uuid == UUID_CONTACT_CHANGED) {
		//ContactChanged &cc = static_cast<ContactChanged &>(e);
		//if(ctx.contains(cc.contact->account))
		//	ctx[cc.contact->account]->
	}
	return true;
}

const QList<GlobalStatus> JabberProto::statuses() const {
	return QList<GlobalStatus>() 
		<< ST_OFFLINE
		<< ST_DND
		<< ST_LONGAWAY
		<< ST_SHORTAWAY
//		<< ST_FREETOCHAT
//		<< ST_INVISIBLE
		<< ST_ONLINE
	;
}

void JabberProto::add_contact(const QString &account_id, const QString &contact_id) {
	Account *acc = accounts_i->account_info(this, account_id);
	if(ctx.contains(acc)) {
		ctx[acc]->addContact(contact_id);
	} else
		qWarning() << "add_contact: no account called" << account_id;

}

bool JabberProto::direct_send(const QString &account_id, const QString &text) {
	Account *acc = accounts_i->account_info(this, account_id);
	if(ctx.contains(acc)) {
		return ctx[acc]->directSend(text);
	} else {
		qWarning() << "direct_send: no account called" << account_id;
		return false;
	}
}

bool JabberProto::gateway_register(const QString &account_id, const QString &gateway) {
	Account *acc = accounts_i->account_info(this, account_id);
	if(ctx.contains(acc)) {
		return ctx[acc]->gatewayRegister(gateway);
	} else {
		qWarning() << "gateway_register: no account called" << account_id;
		return false;
	}
}

bool JabberProto::gateway_unregister(const QString &account_id, const QString &gateway) {
	Account *acc = accounts_i->account_info(this, account_id);
	if(ctx.contains(acc)) {
		return ctx[acc]->gatewayUnregister(gateway);
	} else {
		qWarning() << "gateway_unregister: no account called" << account_id;
		return false;
	}
}

bool JabberProto::remove_account_data(Account *acc) {
	if(ctx.contains(acc)) {
		if(gateways) gateways->remove_account(acc->account_id);
		if(send_direct) send_direct->remove_account(acc);
		delete ctx[acc];
		ctx.remove(acc);
		return true;
	}

	return false;
}

bool JabberProto::update_account_data(Account *acc) {
	if(ctx.contains(acc)) {
		ctx[acc]->setAccountInfo(acc);
	} else {
		ctx[acc] = new JabberCtx(acc, core_i, this);
		connect_context(ctx[acc]);
	}
	if(acc->status == ST_OFFLINE) {
		if(gateways) gateways->remove_account(acc->account_id);
		if(send_direct) send_direct->remove_account(acc);
	} else {
		if(send_direct) send_direct->add_account(acc);
	}

	return true;
}

void JabberProto::parse_extra_data(QDomElement &node, Account *acc) {
	if(ctx.contains(acc)) {
		ctx[acc]->setUseSSL(node.attribute("useSSL", "false") == "true", node.attribute("ignoreSSLErrors", "false") == "true");
		ctx[acc]->setConnectionHost(node.attribute("connectionHost", ""));
	}
}

void JabberProto::set_extra_data(QDomElement &node, Account *acc) {
	if(ctx.contains(acc)) {
		node.setAttribute("useSSL", ctx[acc]->getUseSSL() ? "true" : "false");
		node.setAttribute("ignoreSSLErrors", ctx[acc]->getIgnoreSSLErrors() ? "true" : "false");
		if(!ctx[acc]->getConnectionHost().isEmpty())
			node.setAttribute("connectionHost", ctx[acc]->getConnectionHost());
	}
}

AccountExtra *JabberProto::create_account_extra(Account *acc) {
	ProtoOptions *po = new ProtoOptions(this);
	po->set_info(acc);
	return po;
}

void JabberProto::setUseSSL(Account *account, bool on, bool ignoreErrors) {
	if(ctx.contains(account))
		ctx[account]->setUseSSL(on, ignoreErrors);
}

void JabberProto::setConnectionHost(Account *account, const QString &host) {
	if(ctx.contains(account))
		ctx[account]->setConnectionHost(host);
}

bool JabberProto::getUseSSL(Account *account) {
	if(ctx.contains(account))
		return ctx[account]->getUseSSL();
	return false;
}

bool JabberProto::getIgnoreSSLErrors(Account *account) {
	if(ctx.contains(account))
		return ctx[account]->getIgnoreSSLErrors();
	return false;
}

QString JabberProto::getConnectionHost(Account *account) {
	if(ctx.contains(account))
		return ctx[account]->getConnectionHost();
	return "";
}

ProtoSearchWindowI *JabberProto::create_search_window() {
	return new JabberSearchWin(this);
}

///////////////////////////////////////////

Q_EXPORT_PLUGIN2(jabberPlugin, jabber)
