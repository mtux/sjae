#ifndef JABBER_H
#define JABBER_H

#include <accounts_i.h>
#include <main_window_i.h>
#include <QPointer>
#include <QMap>
#include "jabberctx.h"
#include "protooptions.h"
#include "senddirect.h"
#include "servicediscovery.h"
#include "gatewaylist.h"
#include "asksubscribe.h"

class jabber;

class JabberProto: public ProtocolI {
	Q_OBJECT

public:
	JabberProto(CoreI *core);
	~JabberProto();

	QMenu *getMenu() {return menu;}

	void modules_loaded();
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
	void parse_extra_data(QDomElement &node, Account *account);
	void set_extra_data(QDomElement &node, Account *account);

	// return 0 if you don't have extra options for accounts
	AccountExtra *create_account_extra(Account *account);
	ProtoSearchWindowI *create_search_window();

	bool remove_account_data(Account *account);
	bool update_account_data(Account *account);

	const QList<GlobalStatus> statuses() const;
	const GlobalStatus closest_status_to(GlobalStatus gs) const;

	bool event_fired(EventsI::Event &e);

	void setUseSSL(Account *account, bool on, bool ignoreSSLErrors = false);
	void setConnectionHost(Account *account, const QString &host);

	bool getUseSSL(Account *account);
	bool getIgnoreSSLErrors(Account *account);
	QString getConnectionHost(Account *account);
public slots:
	void add_contact(const QString &account_id, const QString &contact_id);

	// send text direct to the server for the given account, return false if not connected
	bool direct_send(const QString &account_id, const QString &text);
	bool gateway_register(const QString &account_id, const QString &gateway);
	bool gateway_unregister(const QString &account_id, const QString &gateway);

protected slots:
	void contextQueryDiscoInfo(const QString &account_id, const QString &entity_jid, const QString &node = "");

	void handleGranted(const QString &contact_id, const QString &account_id);
	void handleDenied(const QString &contact_id, const QString &account_id);

protected:
	CoreI *core_i;
	QPointer<AccountsI> accounts_i;
	QPointer<EventsI> events_i;
	QPointer<MainWindowI> main_win_i;

	QMap<Account *, JabberCtx *> ctx;
	ServiceDiscovery *service_discovery;
	GatewayList *gateways;
	AskSubscribe *ask_subscribe;
	SendDirect *send_direct;

	QMenu *menu;

	void connect_context(JabberCtx *);
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

protected:
	CoreI *core_i;
};

#endif // JABBER_H
