#ifndef _ROSTER_INC
#define _ROSTER_INC

#include <QStringList>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <QDebug>
#include <QTextDocument> // for Qt::escape function

#include <contact_info_i.h>

typedef enum {ST_NONE, ST_TO, ST_FROM, ST_BOTH, ST_UNKNOWN} SubscriptionType;
typedef enum {PT_UNAVAILABLE, PT_INVISIBLE, PT_ONLINE, PT_AWAY, PT_DND, PT_XA, PT_CHAT, PT_ERROR, PT_UNKNOWN} PresenceType;
typedef enum {RTNT_UNKNOWN, RTNT_GROUP, RTNT_ITEM, RTNT_RESOURCE} NodeType;

class RosterTreeNonLeafNode;

class RosterTreeNode {
public:
	RosterTreeNode(const QString &n, RosterTreeNonLeafNode *p = 0): name(n), parent(p) {}
	RosterTreeNode(RosterTreeNonLeafNode *p = 0): parent(p) {}
	virtual ~RosterTreeNode() {}

	virtual NodeType type() const {return RTNT_UNKNOWN;}

	QString getName() const {return name;}
	virtual void setName(const QString &n) {name = n;}

	RosterTreeNonLeafNode *getParent() const {return parent;}
	virtual void setParent(RosterTreeNonLeafNode *p) {parent = p;}

	virtual int childCount() const {return 0;}

	virtual QString getLabel() const {return name;}

	virtual QVariant getProperty(const QString &key) const {return properties.value(key);}
	virtual void setProperty(const QString &key, const QVariant &v) {properties[key] = v;}

protected:
	QString name;
	RosterTreeNonLeafNode *parent;
	QMap<QString, QVariant> properties;
};

class RosterTreeNonLeafNode: public RosterTreeNode {
public:
	RosterTreeNonLeafNode(const QString &n, RosterTreeNonLeafNode *p = 0): RosterTreeNode(n, p) {}
	virtual ~RosterTreeNonLeafNode() {
		qDeleteAll(children);
	}

	virtual void addChild(RosterTreeNode *child) {
		child->setParent(this); children.append(child);
	}

	virtual bool removeChild(RosterTreeNode *child) {
		if(children.contains(child)) {
			child->setParent(0);
			children.remove(children.indexOf(child));
			return true;
		}
		return false;
	}

	int childCount() const {return children.size();}
	RosterTreeNode *child(int n) const {return children.at(n);}
	RosterTreeNode *child(const QString &name) const {
		QVectorIterator<RosterTreeNode *> i(children);
		RosterTreeNode *item;
		while(i.hasNext()) {
			item = i.next();
			if(item->getName() == name) 
				return item;
		}
		return 0;
	}
	int indexOfChild(RosterTreeNode *child) const {return children.indexOf(child);}

	virtual void clear() {
		qDeleteAll(children);
		children.clear();
	}
protected:
	QVector<RosterTreeNode *> children;
};

class RosterItem;

class Resource: public RosterTreeNode {
public:
	Resource(const QString &n, RosterTreeNonLeafNode *i = 0): RosterTreeNode(n, i), presence(PT_UNAVAILABLE), priority(0) {}
	virtual ~Resource() {}
	virtual NodeType type() const {return RTNT_RESOURCE;}
	PresenceType getPresence() {return presence;}
	QString getPresenceMessage() {return presenceMessage;}
	int getPriority() {return priority;}
	QDateTime getLastActivity() {return last_activity;}

	void setPresence(PresenceType p, const QString &m = "") {presence = p; presenceMessage = m;}
	void setPresenceMessage(const QString &msg) {presenceMessage = msg;}
	void setPriority(int prio) {priority = prio;}
	void updateLastActivity() {last_activity = QDateTime::currentDateTime();}
	
	static PresenceType string2pres(const QString &p) {
		if(p == "unavailable") return PT_UNAVAILABLE;
		else if(p == "invisible") return PT_INVISIBLE;
		else if(p == "available") return PT_ONLINE;
		else if(p == "away") return PT_AWAY;
		else if(p == "dnd") return PT_DND;
		else if(p == "xa") return PT_XA;
		else if(p == "chat") return PT_CHAT;
		else if(p == "error") return PT_ERROR;
		else {
			qDebug() << "failed to parse presence string:" << p;
			return PT_UNKNOWN;
		}
	}

	static QString pres2string(PresenceType p) {
		switch(p) {
			case PT_UNAVAILABLE: return "unavailable";
			case PT_INVISIBLE: return "invisible";
			case PT_ONLINE: return "available";
			case PT_AWAY: return "away";
			case PT_DND: return "dnd";
			case PT_XA: return "xa";
			case PT_CHAT: return "chat";
			case PT_ERROR: return "error";
			default: return "unknown";
		}
	}

	virtual RosterItem *getItem() const;

	virtual QString full_jid() const;
protected:
	PresenceType presence;
	QString presenceMessage;
	int priority;
	QDateTime last_activity;
};

class RosterGroup;

class RosterItem: public RosterTreeNonLeafNode {
public:
	RosterItem(Contact *contact, const QString &name, SubscriptionType sub, RosterGroup *g);

	virtual ~RosterItem() {}
	virtual NodeType type() const {return RTNT_ITEM;}

	virtual void setName(const QString &n);
	virtual void setParent(RosterTreeNonLeafNode *p);

	QString getJID() const {return contact->contact_id;}

	SubscriptionType getSubscription() const {return subscription;}
	void setSubscription(SubscriptionType sub) {subscription = sub;}

	ChatStateType getUserChatState() const {return userChatState;}
	void setUserChatState(ChatStateType t) {userChatState = t;}
	ChatStateType getContactChatState() const {return contactChatState;}
	void setContactChatState(ChatStateType t) {contactChatState = t;}

	static SubscriptionType string2sub(const QString &s) {
		if(s == "none") return ST_NONE;
		else if(s == "to") return ST_TO;
		else if(s == "from") return ST_FROM;
		else if(s == "both") return ST_BOTH;
		else return ST_UNKNOWN;
	}

	static QString sub2string(SubscriptionType s) {
		switch(s) {
			case ST_NONE: return "none";
			case ST_TO: return "to";
			case ST_FROM: return "from";
			case ST_BOTH: return "both";
			default: return "unknown";
		}
	}

	RosterGroup *getGroup() const;
	Resource *get_active_resource() const;
	void setAllResourcePresence(PresenceType pres, const QString &msg);

	Contact *getContact() {return contact;}
	void setAllUnavailable();

	bool is_offline() const;
protected:
	SubscriptionType subscription;
	ChatStateType userChatState, contactChatState;
	Contact *contact;
};

class RosterGroup: public RosterTreeNonLeafNode {
public:
	RosterGroup(const QString &n, RosterGroup *p = 0): RosterTreeNonLeafNode(n, p) {}
	virtual ~RosterGroup() {}
	virtual NodeType type() const {return RTNT_GROUP;}

	static void setDelimiter(const QString &d) {delimiter = d;}
	static QString getDelimiter() {return delimiter;}

	RosterGroup *get_group(const QStringList &group, bool create = true);
	RosterItem *get_item(const QString &jid) const;

	virtual QStringList getClistName() const {
		if(parent)
			return static_cast<RosterGroup *>(parent)->getClistName() << name;
		return QStringList();
	}

	QStringList all_items() {
		QStringList ret;
		foreach(RosterTreeNode *n, children) {
			if(n->type() == RTNT_ITEM) ret << static_cast<RosterItem *>(n)->getJID();
			else if(n->type() == RTNT_GROUP) ret << static_cast<RosterGroup *>(n)->all_items();
		}
		return ret;
	}

protected:
	static QString delimiter;
};

class Roster: public RosterGroup {
public:
	Roster(): RosterGroup("") {}
	virtual ~Roster() {}

	Resource *get_resource(const QString &full_jid, bool create = true);

	static QString full_jid2jid(const QString &full_jid) {
		int index;
		if((index = full_jid.indexOf("/")) == -1)
			return full_jid;
		return full_jid.left(index);
	}

	QStringList getClistName() const {
		return QStringList();
	}
};

#endif
