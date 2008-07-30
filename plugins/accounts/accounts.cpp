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

accounts::accounts(): options(0)
{

}

accounts::~accounts()
{

}

bool accounts::load(CoreI *core) {
	core_i = core;
	if((icons_i = (IconsI *)core_i->get_interface(INAME_ICONS)) == 0) return false;

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

	AccountInfo info;
	info.proto = proto;
	info.enabled = node.attribute("enabled", "true") != "false";
	info.host = node.attribute("host", proto->defaultHost());
	info.nick = node.attribute("nickname", QDir::home().dirName());
	info.username = node.attribute("username", QDir::home().dirName());
	info.password = core_i->decrypt(node.attribute("password"), info.username + info.host);
	info.port = node.attribute("port").toInt();

	if(!set_account_info(account_id, info)) {
		qDebug() << "Accounts: failed to add account info. Protocol:" << proto_name << "Account ID:" << account_id;
		return false;
	}

	proto->parse_extra_data(node, account_id);

	return true;
}

bool accounts::read_data() {
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

QDomElement accounts::toDOM(QDomDocument &doc, const QString &account_id, const AccountInfo &info) {
	QDomElement ret = doc.createElement("account-data");

	ret.setAttribute("protocol-name", info.proto->name());
	ret.setAttribute("id", account_id);
	ret.setAttribute("enabled", info.enabled ? "true" : "false");
	ret.setAttribute("host", info.host);
	ret.setAttribute("nickname", info.nick);
	ret.setAttribute("password", core_i->encrypt(info.password, info.username + info.host));
	ret.setAttribute("port", info.port);
	ret.setAttribute("username", info.username);

	ProtocolI *proto = get_proto_interface(info.proto->name());
	proto->set_extra_data(ret, account_id);

	return ret;
}

bool accounts::save_data() {
	QDomDocument doc;
	QDomElement root = doc.createElement("accounts");
	doc.appendChild(root);	
	QMapIterator<QString, QMap<QString, AccountInfo> > i(account_list);
	while(i.hasNext()) {
		i.next();
		QMapIterator<QString, AccountInfo> j(i.value());
		while(j.hasNext()) {
			j.next();
			root.appendChild(toDOM(doc, j.key(), j.value()));
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

QStringList accounts::account_ids(const QString &proto_name) const {
	return account_list[proto_name].keys();
}

ProtocolI *accounts::get_proto_interface(const QString &proto_name) const {
	if(!protocols.contains(proto_name)) return 0;
	return protocols[proto_name];
}

AccountInfo accounts::account_info(const QString &proto_name, const QString &id) {
	return account_list[proto_name][id];
}

bool accounts::remove_account(const QString &proto_name, const QString &id) {
	if(account_list.contains(proto_name) && account_list[proto_name].contains(id)) {
		account_list[proto_name][id].proto->remove_account_data(id);
		account_list[proto_name].remove(id);
		emit account_removed(proto_name, id);
		return true;
	}
	return false;
}

bool accounts::set_account_info(const QString &id, const AccountInfo &info) {
	bool new_account = !account_list[info.proto->name()].contains(id);

	if(new_account) icons_i->setup_account_status_icons(info.proto, id);		
	account_list[info.proto->name()][id] = info;
	info.proto->update_account_data(id, info);
	if(new_account) emit account_added(info.proto->name(), id);
	emit account_changed(info.proto->name(), id);

	return true;
}

/////////////////////////////

Q_EXPORT_PLUGIN2(accountsPlugin, accounts)
