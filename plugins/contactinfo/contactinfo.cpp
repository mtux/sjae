#include "contactinfo.h"
#include <QtPlugin>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>

#define DB_FILE_NAME		"contact_info.db"

#define GET_PROPS_QUERY				"SELECT parameter, value FROM contact_information WHERE contact_hash_id=:hash;"
#define REPLACE_PROP_QUERY			"REPLACE INTO contact_information VALUES(:hash, :param, :v);"
#define DELETE_PROP_QUERY			"DELETE FROM contact_information WHERE contact_hash_id=:hash AND parameter=:param;"
#define DELETE_PROPS_QUERY			"DELETE FROM contact_information WHERE contact_hash_id=:hash;"


PluginInfo info = {
	0x240,
	"ContactInfo",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"ContactInfo",
	0x00000001
};

ContactInfo::ContactInfo()
{

}

ContactInfo::~ContactInfo()
{

}

bool ContactInfo::load(CoreI *core) {
	core_i = core;
	if((events_i = (EventsI*)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	if((accounts_i = (AccountsI*)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;

	events_i->add_event_listener(this, UUID_CONTACT_CHANGED);

	db = QSqlDatabase::addDatabase("QSQLITE", "ContactInfo");
	db.setDatabaseName(core_i->get_config_dir() + "/" + DB_FILE_NAME);
    if(!db.open()) return false;

	db.exec("CREATE TABLE contact_information ("
		"  contact_hash_id varchar(256),"
		"  parameter varchar(256),"
		"  value text,"
		"  PRIMARY KEY (contact_hash_id));");

	db.exec("CREATE TABLE contact_hash ("
		"  contact_hash_id varchar(256),"
		"  protocol varchar(256),"
		"  account varchar(256),"
		"  contact_id varchar(256),"
		"  PRIMARY KEY (contact_hash_id));");

	get_props = new QSqlQuery(db);
	replace_prop = new QSqlQuery(db);
	delete_prop = new QSqlQuery(db);
	delete_props = new QSqlQuery(db);

	get_props->prepare(GET_PROPS_QUERY);
	replace_prop->prepare(REPLACE_PROP_QUERY);
	delete_prop->prepare(DELETE_PROP_QUERY);
	delete_props->prepare(DELETE_PROPS_QUERY);

	return true;
}

bool ContactInfo::modules_loaded() {
	return true;
}

bool ContactInfo::pre_shutdown() {
	return true;
}

bool ContactInfo::unload() {
	if(db.isOpen())
		db.close();

	QMapIterator<Account *, QMap<QString, Contact *> > i(contacts);
	while(i.hasNext()) {
		i.next();
		foreach(Contact *contact, i.value().values()) {
			delete contact;
		}
	}
	delete get_props;
	delete replace_prop;
	delete delete_prop;
	delete delete_props;
	return true;
}

const PluginInfo &ContactInfo::get_plugin_info() {
	return info;
}

/////////////////////////////
Contact *ContactInfo::get_contact(Account *acc, const QString &contact_id) {
	if(contacts.contains(acc) && contacts[acc].contains(contact_id))
		return contacts[acc][contact_id];

	Contact *contact = new Contact(acc, contact_id);
	contacts[acc][contact_id] = contact;

	QCryptographicHash hasher(QCryptographicHash::Sha1);
	hasher.addData(acc->proto->name().toUtf8());
	hasher.addData(acc->account_id.toUtf8());
	hasher.addData(contact_id.toUtf8());
	contact->hash_id = hasher.result().toBase64();

	if(!hashMap.contains(contact->hash_id)) {
		QSqlQuery writeInfoQuery(db);
		writeInfoQuery.prepare("INSERT INTO contact_hash VALUES(?, ?, ?, ?);");
		writeInfoQuery.addBindValue(contact->hash_id);
		writeInfoQuery.addBindValue(contact->account->proto->name());
		writeInfoQuery.addBindValue(contact->account->account_id);
		writeInfoQuery.addBindValue(contact->contact_id);
		if(!writeInfoQuery.exec()) {
			//qWarning() << "ContactInfo hash data write failed:" << writeInfoQuery.lastError().text();
		}
		hashMap[contact->hash_id] = contact;
	}

	get_props->bindValue(":hash", contact->hash_id);

	if(!get_props->exec())
		qWarning() << "ContactInfo read properties failed:" << get_props->lastError().text();

	while(get_props->next())
		contact->set_property(get_props->value(0).toString(), get_props->value(1));
	get_props->finish();

	contact->clear_changed_properties();

	return contact;
}

Contact *ContactInfo::get_contact(const QString &contact_hash_id) {
	if(hashMap.contains(contact_hash_id))
		return hashMap[contact_hash_id];

	QSqlQuery qm(db);
	if(!qm.exec("SELECT protocol, account, contact_id FROM contact_hash WHERE contact_hash_id='" + contact_hash_id + "';"))
		qDebug() << "Read contact account data failed:" << qm.lastError().text();
	if(qm.next()) {
		Account *account = accounts_i->account_info(qm.value(0).toString(), qm.value(1).toString());
		if(account) {
			return get_contact(account, qm.value(2).toString());
		}
	}
	return 0;
}

bool ContactInfo::delete_contact(Contact *contact) {
	Account *acc = contact->account;
	QString contact_id = contact->contact_id;
	if(contacts.contains(acc) && contacts[acc].contains(contact_id)) {		
		if(events_i) {
			ContactChanged cc(contact, this);
			cc.removed = true;
			events_i->fire_event(cc);
		}

		delete_props->bindValue(":hash", contact->hash_id);

		if(!delete_props->exec())
			qWarning() << "ContactInfo delete query error:" << delete_props->lastError().text();

		delete_props->finish();

		hashMap.remove(contact->hash_id);

		delete contacts[acc][contact_id];
		contacts[acc].remove(contact_id);

		if(contacts[acc].size() == 0)
			contacts.remove(acc);

		return true;
	}
	return false;
}

bool ContactInfo::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_CONTACT_CHANGED && e.source != this) {
		ContactChanged &cc = static_cast<ContactChanged &>(e);

		QStringList changed_props = cc.contact->get_changed_properties();
		if(changed_props.size() == 0) return true;

		foreach(QString prop, changed_props) {
			if(cc.contact->has_property(prop)) {
				//qDebug() << "replacing prop" << prop;
				replace_prop->bindValue(":hash", cc.contact->hash_id);
				replace_prop->bindValue(":param", prop);
				replace_prop->bindValue(":v", cc.contact->get_property(prop));

				if(!replace_prop->exec())
					qWarning() << "ContactInfo property insert failed:" << replace_prop->lastError().text();

				replace_prop->finish();
			} else {
				delete_prop->bindValue(":hash", cc.contact->hash_id);
				delete_prop->bindValue(":param", prop);

				if(!delete_prop->exec())
					qWarning() << "ContactInfo property delete failed:" << delete_prop->lastError().text();

				delete_prop->finish();
			}
		}

		cc.contact->clear_changed_properties();
	}
	return true;
}

/////////////////////////////

Q_EXPORT_PLUGIN2(contactinfo, ContactInfo)

