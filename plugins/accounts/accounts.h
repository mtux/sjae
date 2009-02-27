#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <accounts_i.h>
#include <icons_i.h>
#include <QPointer>
#include <QMap>
#include <QDomElement>
#include "accountsoptions.h"

class accounts: public AccountsI
{
	Q_OBJECT
public:
	accounts();
	~accounts();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool register_protocol(ProtocolI *proto);
	bool deregister_protocol(ProtocolI *proto);

	QStringList protocol_names() const;
	QStringList account_ids(const QString &proto_name) const;
	ProtocolI *get_proto_interface(const QString &proto_name) const;
	AccountInfo account_info(const QString &proto_name, const QString &id);

	bool remove_account(const QString &proto_name, const QString &id);
	bool set_account_info(const QString &id, const AccountInfo &info);

signals:
	void account_removed(const QString &proto_name, const QString &id);
	void account_added(const QString &proto_name, const QString &id);
	void account_changed(const QString &proto_name, const QString &id);

protected:
	CoreI *core_i;
	QPointer<IconsI> icons_i;
	QMap<QString, ProtocolI *> protocols;
	QMap<QString, QMap<QString, AccountInfo> > account_list;

	bool parse_account_node(QDomElement node);
	bool read_data();
	QDomElement toDOM(QDomDocument &doc, const QString &account_id, const AccountInfo &info);

	AccountsOptions *options;
protected slots:
	bool save_data();
};

#endif // ACCOUNTS_H
