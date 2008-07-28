#ifndef _I_ACCOUNTS_H
#define _I_ACCOUNTS_H

#include "plugin_i.h"
#include "options_i.h"
#include "add_contact_i.h"
#include <QString>
#include <QWidget>
#include "global_status.h"

#define INAME_ACCOUNTS	"AccountsInterface"

class ProtocolI;

class AccountInfo {
public:
	QString username, password, host;
	quint16 port;

	QString nick;
	ProtocolI *proto;
	bool enabled;
};

class AccountExtra: public OptionsPageI {
public:
	AccountExtra(QWidget *parent = 0): OptionsPageI(parent) {}

	void set_info(const AccountInfo &info) {
		set_account_info(info);
		reset();
	}

	virtual void set_account_info(const AccountInfo &info) = 0;
};

class QDomElement;

class ProtocolI: public QObject {
	Q_OBJECT
public:
	virtual const QString name() const = 0;
	// return an empty string to disable
	virtual const QString nick_label() const = 0;
	// return true to allow the user to specify a port
	virtual const bool allowSetPort() const = 0;
	// return true to allow the user to specify a host
	virtual const bool allowSetHost() const = 0;

	virtual const quint16 defaultPort() const = 0;
	virtual const QString defaultHost() const = 0;

	// called when reading/writing account data to XML - override to read/store proto-specific data
	virtual void parse_extra_data(QDomElement &node, const QString &account_id) {}
	virtual void set_extra_data(QDomElement &node, const QString &account_id) {}

	// return 0 if you don't have extra options for accounts
	virtual AccountExtra *create_account_extra(const QString &account_id) {return 0;}
	// return 0 if you don't have 'add contact' functionality
	virtual ProtoSearchWindowI *create_search_window() {return 0;}

	virtual bool remove_account_data(const QString &id) = 0;
	virtual bool update_account_data(const QString &id, const AccountInfo &info) = 0;

	// don't include ST_CONNECTING here
	virtual const QList<GlobalStatus> statuses() const = 0;
	virtual const GlobalStatus closest_status_to(GlobalStatus gs) const = 0;
	virtual const GlobalStatus get_status(const QString &account_id) const = 0;

public slots:
	virtual bool message_send(const QString &account_id, const QString &contact_id, const QString &msg, int id) = 0;
	virtual bool set_status(const QString &account_id, GlobalStatus gs) = 0;

signals:
	void message_ack(int id);
	
	void message_recv(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg);
	void status_change(const QString &proto_name, const QString &account_id, const QString &contact_id, GlobalStatus gs);
	void local_status_change(const QString &proto_name, const QString &account_id, GlobalStatus gs);
};

class AccountsI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_ACCOUNTS;}

	virtual bool register_protocol(ProtocolI *proto) = 0;
	virtual bool deregister_protocol(ProtocolI *proto) = 0;

	virtual QStringList protocol_names() const = 0;
	virtual QStringList account_ids(const QString &proto_name) const = 0;
	virtual ProtocolI *get_proto_interface(const QString &proto_name) const = 0;
	virtual AccountInfo account_info(const QString &proto_name, const QString &id) = 0;

	virtual bool remove_account(const QString &proto_name, const QString &id) = 0;
	virtual bool set_account_info(const QString &id, const AccountInfo &info) = 0;
signals:
	void account_removed(const QString &proto_name, const QString &id);
	void account_added(const QString &proto_name, const QString &id);
	void account_changed(const QString &proto_name, const QString &id);
};

#endif
