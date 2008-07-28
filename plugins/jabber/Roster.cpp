#include "Roster.h"
#include <QDebug>

QString RosterGroup::delimiter = "\\";

RosterItem *Resource::getItem() const {
	return static_cast<RosterItem *>(getParent());
}

QString Resource::full_jid() const {
	return getItem()->getJID() + "/" + name;
}

RosterGroup *RosterItem::getGroup() const {
	return static_cast<RosterGroup *>(getParent());
}

Resource *RosterItem::get_active_resource() const {
	if(childCount() == 0) return 0;
	else if(childCount() == 1) return static_cast<Resource *>(child(0));
	else {
		// TODO: get most online or last active or highest priority resource
		return static_cast<Resource *>(child(0));
	}
}

bool RosterItem::is_offline() const {
	QVectorIterator<RosterTreeNode *> i(children);
	Resource *r;
	while(i.hasNext()) {
		r = static_cast<Resource *>(i.next());
		if(r->getPresence() != PT_UNAVAILABLE)
			return false;
	}
	return true;	
}


RosterGroup *RosterGroup::get_group(const QString &group, bool create) {
	if(group.isEmpty())
		return this;

	QString subgroup, rest;
	int index;
	if(!delimiter.isEmpty() && (index = group.indexOf(delimiter)) != -1) {
		subgroup = group.left(index);
		rest = group.mid(index + delimiter.length());
	} else
		subgroup = group;

	QVectorIterator<RosterTreeNode *> i(children);
	RosterGroup *g = 0;
	RosterTreeNode *c = 0;
	while(i.hasNext()) {
		c = i.next();
		if(c->type() == RTNT_GROUP && c->getName() == subgroup) {
			g = static_cast<RosterGroup *>(c);
			break;
		}
	}
	if(!g && create) {
		g = new RosterGroup(subgroup, this);
		addChild(g);
	}

	return g ? g->get_group(rest, create) : 0;
}

RosterItem *RosterGroup::get_item(const QString &jid) const {
	RosterTreeNode *c = 0;
	QVectorIterator<RosterTreeNode *> i(children);
	while(i.hasNext()) {
		c = i.next();
		if(c->type() == RTNT_ITEM) {
			RosterItem *item = static_cast<RosterItem *>(c);
			if(item->getJID() == jid)
				return item;
		} else if(c->type() == RTNT_GROUP) {
			RosterItem *i = static_cast<RosterGroup *>(c)->get_item(jid);
			if(i) return i;
		}
	}
	return 0;
}

Resource *Roster::get_resource(const QString &full_jid, bool create) {
	QString jid, resource;
	int index = full_jid.indexOf("/");
	if(index == -1) {
		jid = full_jid;
		resource = "default";
	} else {
		jid = full_jid.left(index);
		resource = full_jid.mid(index + 1);
	}
	//qDebug() << "jid:" << jid << "resource:" << resource;
	RosterItem *item = get_item(jid);
	if(!item) {
		qDebug() << "no item";
		return 0;
	}

	Resource *n = static_cast<Resource *>(item->child(resource));
	if(!n && create) {
		n = new Resource(resource, item);
		item->addChild(n);
	}

	return n;
}
