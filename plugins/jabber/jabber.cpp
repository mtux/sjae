#include "jabber.h"
#include <QtPlugin>
#include <QDebug>
#include <icons_i.h>

PluginInfo info = {
	0x800,
	"Jabber Protocol",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Jabber protocol plugin for SJC",
	0x00000001
};


jabber::jabber(): proto(0)
{

}

jabber::~jabber()
{

}

bool jabber::load(CoreI *core) {
	core_i = core;
	accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS);
	IconsI *icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);
	if(icons_i) icons_i->add_icon("Proto/Jabber", QPixmap(":/icons/Resources/bulb.PNG"), "Protocols/Jabber");

	proto = new JabberProto(this);
	accounts_i->register_protocol(proto);
	connect(accounts_i, SIGNAL(account_changed(const QString &, const QString &)), proto, SLOT(account_changed(const QString &, const QString &)));
	connect(accounts_i, SIGNAL(account_added(const QString &, const QString &)), proto, SLOT(account_added(const QString &, const QString &)));
	connect(accounts_i, SIGNAL(account_removed(const QString &, const QString &)), proto, SLOT(account_removed(const QString &, const QString &)));
	return true;
}

bool jabber::modules_loaded() {
	//OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	//if(options_i)
	//	options_i->add_page("User Interface/Contact List", new CListOptions(this));

	return true;
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
	}

	ctx[account_id]->requestStatus(gs);

	return true;
}

void JabberProto::context_status_change(const QString &account_id, GlobalStatus gs) {
	emit local_status_change(name(), account_id, gs);
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
	}

	return true;
}

void JabberProto::account_added(const QString &proto_name, const QString &account_id) {
	/*
	if(proto_name != name()) return;

	if(!ctx.contains(account_id)) {
		ctx[account_id] = new JabberCtx(account_id, plugin->accounts_i->account_info(name(), account_id), plugin->core_i, this);
		connect(ctx[account_id], SIGNAL(statusChanged(const QString &, GlobalStatus)), this, SLOT(context_status_change(const QString &, GlobalStatus)));
		connect(ctx[account_id], SIGNAL(msgRecv(const QString &, const QString &, const QString &)), this, SLOT(context_message_recv(const QString &, const QString &, const QString &)));
	}
	*/
}

void JabberProto::account_removed(const QString &proto_name, const QString &account_id) {
}

void JabberProto::account_changed(const QString &proto_name, const QString &account_id) {
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

///////////////////////////////////////////

Q_EXPORT_PLUGIN2(jabberPlugin, jabber)
