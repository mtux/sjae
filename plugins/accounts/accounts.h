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
	ProtocolI *get_proto_interface(const QString &proto_name) const;
	QStringList account_ids(ProtocolI *proto) const;
	QStringList account_ids(const QString &proto_name) const;
	Account *account_info(ProtocolI *proto, const QString &id);
	Account *account_info(const QString &proto_name, const QString &id);

	bool remove_account(Account *acc);
	Account *set_account_info(const Account &acc);

	bool event_fired(EventsI::Event &e);


protected:
	CoreI *core_i;
	QPointer<IconsI> icons_i;
	QPointer<EventsI> events_i;
	QMap<QString, ProtocolI *> protocols;
	QMap<ProtocolI *, QMap<QString, Account *> > account_list;

	bool parse_account_node(QDomElement node);
	bool read_data();
	QDomElement toDOM(QDomDocument &doc, Account *acc);

	AccountsOptions *options;
protected slots:
	bool save_data();
};

#endif // ACCOUNTS_H
