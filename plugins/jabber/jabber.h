#ifndef JABBER_H
#define JABBER_H

#include <accounts_i.h>
#include <main_window_i.h>
#include <QPointer>
#include <QMap>
#include "jabberctx.h"
#include "protooptions.h"
#include "senddirect.h"

class jabber;

class JabberProto: public ProtocolI {
	Q_OBJECT

public:
	JabberProto(jabber *jabberPlugin);
	~JabberProto();

	void deleteContexts();

	const QString name() const {return "Jabber";}
	// return an empty string to disable
	const QString nick_label() const {return QString();}
	// return true to allow the user to specify a port
	const bool allowSetPort() const {return true;}
	// return true to allow the user to specify a host
	const bool allowSetHost() const {return true;}

	const quint16 defaultPort() const {return 5222;}
	const QString defaultHost() const {return "jabber.org";}

	// called when reading/writing account data to XML
	void parse_extra_data(QDomElement &node, const QString &account_id);
	void set_extra_data(QDomElement &node, const QString &account_id);

	// return 0 if you don't have extra options for accounts
	AccountExtra *create_account_extra(const QString &account_id);
	ProtoSearchWindowI *create_search_window();

	bool remove_account_data(const QString &id);
	bool update_account_data(const QString &id, const AccountInfo &info);

	const QList<GlobalStatus> statuses() const;
	const GlobalStatus closest_status_to(GlobalStatus gs) const;
	const GlobalStatus get_status(const QString &account_id) const;

public slots:
	bool message_send(const QString &account_id, const QString &contact_id, const QString &msg, int id);
	bool set_status(const QString &account_id, GlobalStatus gs);

	void add_contact(const QString &account_id, const QString &contact_id);

	// send text direct to the server for the given account, return false if not connected
	bool direct_send(const QString &account_id, const QString &text);

	void account_changed(const QString &account_id);
	void account_added(const QString &account_id);
	void account_removed(const QString &account_id);
protected slots:
	void context_status_change(const QString &account_id, GlobalStatus gs);
	void context_message_recv(const QString &account_id, const QString &contact_id, const QString &msg);


signals:
	void msgAck(int i);
protected:
	jabber *plugin;
	QMap<QString, JabberCtx *> ctx;
};

class jabber: public PluginI
{
	Q_OBJECT
	Q_INTERFACES(PluginI)

	friend class JabberProto;
public:
	jabber();
	~jabber();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	JabberProto *proto;

protected slots:
	void account_changed(const QString &proto_name, const QString &account_id);
	void account_added(const QString &proto_name, const QString &account_id);
	void account_removed(const QString &proto_name, const QString &account_id);

protected:
	CoreI *core_i;
	QPointer<AccountsI> accounts_i;
	QPointer<MainWindowI> main_win_i;

	SendDirect *sd;
};

#endif // JABBER_H
