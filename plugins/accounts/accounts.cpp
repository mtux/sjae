#include "accounts.h"
#include <QtPlugin>
#include <QDebug>
#include <global_status.h>
#include <QFileInfo>
#include <QDir>

#define ACCOUNTS_DATA_FILENAME "accounts.xml"

PluginInfo info = {
	0x200,
	"Account Manager",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Account manager",
	0x00000001
};

bool accounts::event_fired(EventsI::Event &e) {
	return true;
}

accounts::accounts(): options(0)
{

}

accounts::~accounts()
{

}

bool accounts::load(CoreI *core) {
	core_i = core;
	if((icons_i = (IconsI *)core_i->get_interface(INAME_ICONS)) == 0) return false;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;

	return true;
}

bool accounts::modules_loaded() {
	OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	if(options_i) options_i->add_page("Accounts", options = new AccountsOptions(this));
	connect(options, SIGNAL(applied()), this, SLOT(save_data()));

	read_data();

	return true;
}

bool accounts::pre_shutdown() {
	return true;
}

bool accounts::unload() {
	return true;
}

const PluginInfo &accounts::get_plugin_info() {
	return info;
}


/////////////////////////////

bool accounts::parse_account_node(QDomElement node) {
	QString proto_name = node.attribute("protocol-name");
	ProtocolI *proto = get_proto_interface(proto_name);
	if(!proto) {
		qDebug() << "Accounts: skipping account data for unknown protocol -" << proto_name << "- on line" << node.lineNumber();
		return false;
	}
	
	QString account_id = node.attribute("id");
	if(account_ids(proto_name).contains(account_id)) {
		qDebug() << "Accounts: skipping already defined account id -" << account_id << "- on line" << node.lineNumber();
		return false;
	}

	Account *acc = new Account();
	acc->proto = proto;
	acc->enabled = node.attribute("enabled", "true") != "false";
	acc->host = node.attribute("host", proto->defaultHost());
	acc->nick = node.attribute("nickname", QDir::home().dirName());
	acc->username = node.attribute("username", QDir::home().dirName());
	acc->password = core_i->decrypt(node.attribute("password"), acc->username + acc->host);
	acc->port = node.attribute("port").toInt();
	acc->account_id = account_id;
	acc->status = acc->desiredStatus = ST_OFFLINE;

	account_list[acc->proto][acc->account_id] = acc;
	icons_i->setup_account_status_icons(acc);			

	events_i->fire_event(AccountChanged(acc, this));
	proto->parse_extra_data(node, acc);

	return true;
}

bool accounts::read_data() {
	qDebug() << "reading accound data";

	QFile file(core_i->get_config_dir() + "/" + ACCOUNTS_DATA_FILENAME);
	QFileInfo info(file);
	if(!file.exists()) {
		qDebug() << "File not found:" << info.absoluteFilePath();
		return true;
	}

	QString error_str;
	int error_line, error_col;
	QDomDocument doc;
	if(!doc.setContent(&file, false, &error_str, &error_line, &error_col)) {
		qDebug() << "Accounts: failed to load XML file ("<< info.absoluteFilePath() << ") - error on line" << error_line << ":" << error_str;
		return false;
	}
	
	QDomElement root = doc.documentElement();
	if(root.tagName() != "accounts") {
		qDebug() << "Accounts: XML contains no 'accounts' element";
		return false;
	}
	
	QDomNode node = root.firstChild();
	while(!node.isNull()) {
		if(node.toElement().tagName() == "account-data")
			parse_account_node(node.toElement());
		node = node.nextSibling();
	}
	
	if(options) options->reset();

	return true;
}

QDomElement accounts::toDOM(QDomDocument &doc, Account *acc) {
	QDomElement ret = doc.createElement("account-data");

	ret.setAttribute("protocol-name", acc->proto->name());
	ret.setAttribute("id", acc->account_id);
	ret.setAttribute("enabled", acc->enabled ? "true" : "false");
	ret.setAttribute("host", acc->host);
	ret.setAttribute("nickname", acc->nick);
	ret.setAttribute("password", core_i->encrypt(acc->password, acc->username + acc->host));
	ret.setAttribute("port", acc->port);
	ret.setAttribute("username", acc->username);

	acc->proto->set_extra_data(ret, acc);

	return ret;
}

bool accounts::save_data() {
	QDomDocument doc;
	QDomElement root = doc.createElement("accounts");
	doc.appendChild(root);	
	QMapIterator<ProtocolI *, QMap<QString, Account *> > i(account_list);
	while(i.hasNext()) {
		i.next();
		QMapIterator<QString, Account *> j(i.value());
		while(j.hasNext()) {
			j.next();
			root.appendChild(toDOM(doc, j.value()));
		}
	}

	if(!QDir::root().exists(core_i->get_config_dir()))
		QDir::root().mkpath(core_i->get_config_dir());

	QFile file(core_i->get_config_dir() + "/" + ACCOUNTS_DATA_FILENAME);
	QFileInfo info(file);
	if(!file.open(QIODevice::WriteOnly)) {
		qDebug() << "Accounts: could not write XML to" << info.absoluteFilePath();
		return false;
	}
	QTextStream st(&file);
	st << doc.toString();
	file.close();
	return true;
}

/////////////////////////////

bool accounts::register_protocol(ProtocolI *proto) {
	if(!protocols.contains(proto->name())) {
		protocols[proto->name()] = proto;
		if(options) options->reset();
		icons_i->setup_proto_icons(proto);
		return true;
	}
	return false;
}

bool accounts::deregister_protocol(ProtocolI *proto) {
	if(protocols.contains(proto->name())) {
		protocols.remove(proto->name());
		options->reset();
		return true;
	}
	return false;
}
QStringList accounts::protocol_names() const {
	return protocols.keys();
}

ProtocolI *accounts::get_proto_interface(const QString &proto_name) const {
	if(!protocols.contains(proto_name)) return 0;
	return protocols[proto_name];
}

QStringList accounts::account_ids(ProtocolI *proto) const {
	if(proto && account_list.contains(proto))
		return account_list[proto].keys();
	return QStringList();
}

QStringList accounts::account_ids(const QString &proto_name) const {
	ProtocolI *proto = get_proto_interface(proto_name);
	return account_ids(proto);
}

Account *accounts::account_info(ProtocolI *proto, const QString &id) {
	if(account_list.contains(proto) && account_list[proto].contains(id))
		return account_list[proto][id];
	return 0;
}

Account *accounts::account_info(const QString &proto_name, const QString &id) {
	ProtocolI *proto = get_proto_interface(proto_name);
	return account_info(proto, id);
}

bool accounts::remove_account(Account *acc) {
	if(account_list.contains(acc->proto) && account_list[acc->proto].contains(acc->account_id)) {
		AccountChanged ace(acc, this);
		ace.removed = true;
		events_i->fire_event(ace);

		account_list[acc->proto].remove(acc->account_id);

		if(account_list[acc->proto].size() == 0)
			account_list.remove(acc->proto);

		delete acc;

		return true;
	}
	return false;
}

Account *accounts::set_account_info(const Account &acc) {
	bool new_account = account_list.contains(acc.proto) == false || account_list[acc.proto].contains(acc.account_id) == false;
	if(new_account) {
		account_list[acc.proto][acc.account_id] = new Account();
		*account_list[acc.proto][acc.account_id] = acc;
		icons_i->setup_account_status_icons(account_list[acc.proto][acc.account_id]);			
	} else
		*account_list[acc.proto][acc.account_id] = acc;

	events_i->fire_event(AccountChanged(account_list[acc.proto][acc.account_id], this));

	return account_list[acc.proto][acc.account_id];
}

/////////////////////////////

Q_EXPORT_PLUGIN2(accountsPlugin, accounts)
