#ifndef _DISCO_INFO
#define _DISCO_INFO

#include <QString>
#include <QList>
#include <QMetaType>

class Identity {
public:
	QString category, type, name;
};

class Feature {
public:
	QString var;
};

class DiscoInfo
{
public:
	DiscoInfo(void);
	//DiscoInfo(const DiscoInfo &other);
	~DiscoInfo(void);

	QString entity, node;
	QList<Identity> indentities;
	QList<Feature> features;
	QString account_id;
};

class Item {
public:
	QString jid, name, node;
};

class DiscoItems
{
public:
	DiscoItems(void);
	//DiscoItems(const DiscoItems &other);
	~DiscoItems(void);

	QString entity;
	QList<Item> items;
	QString account_id;
};

void registerDiscoMetaTypes();

#endif
