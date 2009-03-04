#ifndef _I_CONTACT_INFO_H
#define _I_CONTACT_INFO_H

#include "accounts_i.h"
#include "events_i.h"

#define INAME_CONTACTINFO		"ContactInfoInterface"

#define UUID_CONTACT_CHANGED		"{3602C4D5-2589-4be5-AC4D-A2DCA5A4F8F0}"
#define UUID_MSG					"{99D59C72-F5C6-4a20-84CF-1BAA6612E945}"
#define UUID_MSG_SEND				"{1660647B-A4B3-4138-BFB6-67DE1D8F44C9}"
#define UUID_CHAT_STATE				"{AC8F9710-54B5-40af-9CA1-779AB4ED59DB}"

class Contact {
public:
	Contact(Account *a, const QString &id): account(a), contact_id(id), status(ST_OFFLINE) {}

	Account *account;
	QString contact_id;

	QString hash_id;

	GlobalStatus status;
	QVariant get_property(const QString &key) {if(properties.contains(key)) return properties[key]; return QVariant();}
	void set_property(const QString &key, const QVariant &v) {
		if((!properties.contains(key) || properties[key] != v) && changed_properties.indexOf(key) == -1 && transient_properties.indexOf(key) == -1)
			changed_properties.append(key);
		properties[key] = v;
	}
	QStringList get_changed_properties() {return changed_properties;}
	void clear_changed_properties() {changed_properties.clear();}
	void remove_property(const QString &key) {
		if(properties.contains(key)) {
			properties.remove(key);
			if(changed_properties.indexOf(key) == -1 && transient_properties.indexOf(key) == -1)
				changed_properties.append(key);
		}
	}
	bool has_property(const QString &key) {
		return properties.contains(key);
	}

	void mark_transient(const QString &key) {
		if(!transient_properties.contains(key)) {
			transient_properties.append(key);
		}
	}
private:
	QMap<QString, QVariant> properties; // should be property cache?
	QStringList changed_properties;
	QStringList transient_properties;
};

class ContactChanged: public EventsI::Event {
public:
	ContactChanged(Contact *c, QObject *source = 0): EventsI::Event(UUID_CONTACT_CHANGED, source), contact(c), removed(false) {}
	Contact *contact;
	bool removed;
};

class Message: public EventsI::Event {
public:
	Message(QObject *source = 0): EventsI::Event(UUID_MSG, source), contact(0), id(-1), read(false) {}
	Message(Contact *c, const QString &msg, bool incomming, int i, QObject *source = 0): 
				EventsI::Event(UUID_MSG, source, (incomming ? EventsI::ET_INCOMMING : EventsI::ET_OUTGOING)), contact(c), id(i), text(msg), read(!incomming)   {}
	Message(const Message &m): EventsI::Event(UUID_MSG, m.source, m.type), contact(m.contact), id(m.id), text(m.text), read(m.read) {
		timestamp = m.timestamp;
	}
	Contact *contact;
	int id;
	QString text;
	bool read;
};

class MessageSendReq: public EventsI::Event {
public:
	MessageSendReq(Contact *c, const QString &msg, int i, QObject *source = 0):
				EventsI::Event(UUID_MSG_SEND, source, EventsI::ET_INTERNAL), contact(c), id(i), text(msg)   {}
	Contact *contact;
	int id;
	QString text;
};

typedef enum {CS_ACTIVE, CS_COMPOSING, CS_PAUSED, CS_INACTIVE, CS_GONE}  ChatStateType;

class ChatState: public EventsI::Event {
public:

	ChatState(Contact *c, ChatStateType type, bool incomming, QObject *source = 0): 
		EventsI::Event(UUID_CHAT_STATE, source, (incomming ? EventsI::ET_INCOMMING : EventsI::ET_OUTGOING)), contact(c), state_type(type) {}
	Contact *contact;
	ChatStateType state_type;
};

class ContactInfoI: public PluginI, public EventsI::EventListener {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	const QString get_interface_name() const {return INAME_CONTACTINFO;}

	virtual Contact *get_contact(Account *acc, const QString &contact_id) = 0;
	virtual bool delete_contact(Contact *contact) = 0;
};

#endif
