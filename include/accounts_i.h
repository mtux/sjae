#ifndef _I_ACCOUNTS_H
#define _I_ACCOUNTS_H

#include "plugin_i.h"
#include "options_i.h"
#include "add_contact_i.h"
#include "events_i.h"
#include "global_status.h"
#include <QString>
#include <QWidget>
#include <QMap>
#include <QVariant>

#define INAME_ACCOUNTS	"AccountsInterface"

// event UUIDs
#define UUID_MSG_RECV				"{DB769558-BF1F-41a8-83F9-C4374F5B613A}"
#define UUID_MSG_SEND				"{4C514167-A03A-4a9c-AE92-0E1E1471AAEF}"
#define UUID_CONTACT_CHANGED		"{3602C4D5-2589-4be5-AC4D-A2DCA5A4F8F0}"
#define UUID_ACCOUNT_CHANGED		"{33A2C7A3-2F82-46b1-B56E-511D9A6D7ED2}"
#define UUID_ACCOUNT_STATUS_REQ		"{1DCBB288-0686-40bb-9118-C184D9125A9F}"

#define UUID_USER_CHAT_STATE		"{AC8F9710-54B5-40af-9CA1-779AB4ED59DB}"
#define UUID_CONTACT_CHAT_STATE		"{C00E8853-CF13-4a7d-AF60-2684FF4D8FE9}"

class ProtocolI;

class Account {
public:
	Account(): proto(0), status(ST_OFFLINE), desiredStatus(ST_OFFLINE) {}

	QString account_id;

	QString username, password, host;
	quint16 port;

	QString nick;
	ProtocolI *proto;
	bool enabled;

	GlobalStatus status, desiredStatus;
};

class AccountExtra: public OptionsPageI {
public:
	AccountExtra(QWidget *parent = 0): OptionsPageI(parent) {}

	void set_info(Account *acc) {
		set_account_info(acc);
		reset();
	}

	virtual void set_account_info(Account *acc) = 0;
};

class Contact {
public:
	Contact(Account *a, const QString &id): account(a), contact_id(id), status(ST_OFFLINE) {}

	Account *account;
	QString contact_id;

	GlobalStatus status;
	QMap<QString, QVariant> properties; // should be property cache?
};

class MessageSend: public EventsI::Event {
public:
	MessageSend(const QString &msg, int i, Contact *c, QObject *source = 0): EventsI::Event(UUID_MSG_SEND, source), message(msg), id(i), contact(c) {}
	QString message;
	Contact *contact;
	int id;
};

class MessageRecv: public EventsI::Event {
public:
	MessageRecv(const QString &msg, int i, Contact *c, QObject *source = 0): EventsI::Event(UUID_MSG_RECV, source), message(msg), id(i), contact(c) {}
	QString message;
	Contact *contact;
	int id;
};

class ContactChanged: public EventsI::Event {
public:
	ContactChanged(Contact *c, QObject *source = 0): EventsI::Event(UUID_CONTACT_CHANGED, source), contact(c), removed(false) {}
	Contact *contact;
	bool removed;
};

class AccountChanged: public EventsI::Event {
public:
	AccountChanged(Account *a, QObject *source = 0): EventsI::Event(UUID_ACCOUNT_CHANGED, source), account(a), removed(false) {}
	Account *account;
	bool removed;
};

class AccountStatusReq: public EventsI::Event {
public:
	AccountStatusReq(Account *a, GlobalStatus gs, QObject *source = 0): EventsI::Event(UUID_ACCOUNT_STATUS_REQ, source), account(a), status(gs) {}
	Account *account;
	GlobalStatus status;
};

typedef enum {CS_ACTIVE, CS_COMPOSING, CS_PAUSED, CS_INACTIVE, CS_GONE}  ChatStateType;

class UserChatState: public EventsI::Event {
public:

	UserChatState(Contact *c, ChatStateType t, QObject *source = 0): 
		EventsI::Event(UUID_USER_CHAT_STATE, source), contact(c), type(t) {}
	Contact *contact;
	ChatStateType type;
};

class ContactChatState: public EventsI::Event {
public:
	ContactChatState(Contact *c, ChatStateType t, QObject *source = 0): 
		EventsI::Event(UUID_CONTACT_CHAT_STATE, source), contact(c), type(t) {}
	Contact *contact;
	ChatStateType type;
};

class QDomElement;

class ProtocolI: public QObject, public EventsI::EventListener {
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
	virtual void parse_extra_data(QDomElement &node, Account *acc) {}
	virtual void set_extra_data(QDomElement &node, Account *acc) {}

	// return 0 if you don't have extra options for accounts
	virtual AccountExtra *create_account_extra(Account *acc) {return 0;}
	// return 0 if you don't have 'add contact' functionality
	virtual ProtoSearchWindowI *create_search_window() {return 0;}

	virtual bool remove_account_data(Account *acc) = 0;
	virtual bool update_account_data(Account *acc) = 0;

	// don't include ST_CONNECTING here
	virtual const QList<GlobalStatus> statuses() const = 0;
	virtual const GlobalStatus closest_status_to(GlobalStatus gs) const = 0;

	virtual bool event_fired(EventsI::Event &e) = 0;
};

class AccountsI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_ACCOUNTS;}

	virtual bool register_protocol(ProtocolI *proto) = 0;
	virtual bool deregister_protocol(ProtocolI *proto) = 0;

	virtual QStringList protocol_names() const = 0;
	virtual ProtocolI *get_proto_interface(const QString &proto_name) const = 0;
	virtual QStringList account_ids(ProtocolI *proto) const = 0;
	virtual QStringList account_ids(const QString &proto_name) const = 0;
	virtual Account *account_info(ProtocolI *proto, const QString &id) = 0;
	virtual Account *account_info(const QString &proto_name, const QString &id) = 0;

	virtual bool remove_account(Account *acc) = 0;
	virtual Account *set_account_info(const Account &acc) = 0;
};

#endif
