#include "history.h"
#include <QtPlugin>
#include <QDebug>
#include <QSqlError>
#include <QCryptographicHash>

#define DB_FILE_NAME		"message_history.db"

double timestamp_encode(const QDateTime &d) {
	return d.toTime_t() + d.time().msec() / 1000.0;
}

QDateTime timestamp_decode(const double val) {
	uint secs = (uint)val;
	int msecs = (int)((val - secs) * 1000 + 0.5);
	return QDateTime::fromTime_t(secs).addMSecs(msecs);
}

PluginInfo info = {
	0x280,
	"History",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"History",
	0x00000001
};

History::History()
{

}

History::~History()
{

}

bool History::load(CoreI *core) {
	core_i = core;
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	if((contact_info_i = (ContactInfoI*)core_i->get_interface(INAME_CONTACTINFO)) == 0) return false;
	if((accounts_i = (AccountsI*)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;

	events_i->add_event_filter(this, 0x100, UUID_MSG);

	db = QSqlDatabase::addDatabase("QSQLITE", "History");
	db.setDatabaseName(core_i->get_config_dir() + "/" + DB_FILE_NAME);
    if(!db.open()) return false;

	QSqlQuery q(db);

	if(!q.exec("CREATE TABLE message_history ("
		"  contact_hash_id varchar(256),"
		"  timestamp number,"
		"  incomming boolean,"
		"  msg_read boolean,"
		"  message text);"))
	{
		qWarning() << "History db error:" << q.lastError().text();
	}

	if(!q.exec("CREATE TABLE contact_hash ("
		"  contact_hash_id varchar(256),"
		"  protocol varchar(256),"
		"  account varchar(256),"
		"  contact_id varchar(256));"))
	{
		qWarning() << "History db error:" << q.lastError().text();
	}


	return true;
}

Contact *History::get_contact(const QString &contact_hash) {
	if(hashMap.contains(contact_hash))
		return hashMap[contact_hash];

	QSqlQuery qm(db);
	if(!qm.exec("SELECT protocol, account, contact_id FROM contact_hash WHERE contact_hash_id='" + contact_hash + "';"))
		qDebug() << "Read contact account data failed:" << qm.lastError().text();
	if(qm.next()) {
		Account *account = accounts_i->account_info(qm.value(0).toString(), qm.value(1).toString());
		if(account) {
			Contact *contact = contact_info_i->get_contact(account, qm.value(2).toString());
			hashMap[contact->hash_id] = contact;
			return contact;
		}
	}
	return 0;
}

bool History::modules_loaded() {
	QSqlQuery unread(db);
	if(!unread.exec("SELECT contact_hash_id, message, timestamp FROM message_history WHERE incomming='true' AND msg_read='false';"))
		qWarning() << "History read unread failed:" << unread.lastError().text();

	while(unread.next()) {
		Contact *contact = get_contact(unread.value(0).toString());
		if(contact) {
			ContactChanged cc(contact, this);
			events_i->fire_event(cc);
			Message m(contact, unread.value(1).toString(), true, 0, this);
			m.timestamp = timestamp_decode(unread.value(2).toDouble());
			events_i->fire_event(m);
		}
	}
	return true;
}

bool History::pre_shutdown() {
	events_i->remove_event_listener(this, UUID_MSG);
	return true;
}

bool History::unload() {
	if(db.isOpen())
		db.close();
	return true;
}

const PluginInfo &History::get_plugin_info() {
	return info;
}

/////////////////////////////

bool History::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_MSG) {
		Message &m = static_cast<Message &>(e);
		double t = timestamp_encode(m.timestamp);
		if(m.source != this) {
			if(m.contact->has_property("DisableHistory") && m.contact->get_property("DisableHistory").toBool() == true)
				return true;

			if(!hashMap.contains(m.contact->hash_id)) {
				QSqlQuery writeInfoQuery(db);
				writeInfoQuery.prepare("INSERT INTO contact_hash VALUES(?, ?, ?, ?);");
				writeInfoQuery.addBindValue(m.contact->hash_id);
				writeInfoQuery.addBindValue(m.contact->account->proto->name());
				writeInfoQuery.addBindValue(m.contact->account->account_id);
				writeInfoQuery.addBindValue(m.contact->contact_id);
				if(!writeInfoQuery.exec()) {
					qWarning() << "History write info failed:" << writeInfoQuery.lastError().text();
				}
				hashMap[m.contact->hash_id] = m.contact;
			}

			QSqlQuery writeMessageQuery(db);
			writeMessageQuery.prepare("INSERT INTO message_history VALUES(?, ?, ?, ?, ?);");

			writeMessageQuery.addBindValue(m.contact->hash_id);
			writeMessageQuery.addBindValue(t);
			writeMessageQuery.addBindValue(m.type == EventsI::ET_INCOMMING);
			writeMessageQuery.addBindValue(m.read);
			writeMessageQuery.addBindValue(m.text);

			if(!writeMessageQuery.exec()) {
				qWarning() << "History write failed:" << writeMessageQuery.lastError().text();
			}

		}

		if(!m.read) {
			if(m.contact->has_property("PendingMsg")) {
				m.contact->set_property("PendingMsg", m.contact->get_property("PendingMsg").toList() << t);
			} else {
				m.contact->mark_transient("PendingMsg");
				m.contact->set_property("PendingMsg", QList<QVariant>() << t);
				ContactChanged cc(m.contact, this);
				events_i->fire_event(cc);
			}
		}
	}
	return true;
}

QList<Message> History::read_history(QSqlQuery &query, bool mark_read) {
	if(!query.exec()) {
		qWarning() << "History read failed:" << query.lastError().text();
	}

	QList<Message> ret;
	Contact *contact;
	double t;
	while(query.next()) {
		contact = get_contact(query.value(0).toString());
		if(contact) {
			Message m(contact, query.value(1).toString(), query.value(2).toBool(), 0, this);
			m.read = query.value(3).toBool();
			t = query.value(4).toDouble();
			m.timestamp = timestamp_decode(t);
			if(!m.read && mark_read)
				mark_as_read(contact, t);
			ret << m;
		}
	}

	qSort(ret);
	return ret;
}

void History::refire_latest_events(Contact *contact, QDateTime earliest, bool mark_read) {

	QSqlQuery readQuery(db);
	readQuery.prepare("SELECT contact_hash_id, message, incomming, msg_read, timestamp FROM message_history WHERE contact_hash_id=:hash AND timestamp>=:timestamp ORDER BY timestamp ASC;");

	readQuery.bindValue(":hash", contact->hash_id);
	readQuery.bindValue(":timestamp", timestamp_encode(earliest));

	QList<Message> event_list = read_history(readQuery, mark_read);
	foreach(Message m, event_list)
		events_i->fire_event(m);
}

void History::refire_latest_events(Contact *contact, int count, bool mark_read) {

	QSqlQuery readQuery(db);
	readQuery.prepare("SELECT contact_hash_id, message, incomming, msg_read, timestamp FROM message_history WHERE contact_hash_id=:hash ORDER BY timestamp DESC LIMIT :count;");

	readQuery.bindValue(":hash", contact->hash_id);
	readQuery.bindValue(":count", count);

	QList<Message> event_list = read_history(readQuery, mark_read);
	foreach(Message m, event_list)
		events_i->fire_event(m);
}

void History::refire_latest_events(QList<Contact *> contacts, QDateTime earliest, bool mark_read) {
	QString contactsQueryPart;
	foreach(Contact *contact, contacts) {
		if(contactsQueryPart.size())
			contactsQueryPart += " OR ";
		contactsQueryPart += "contact_hash_id='" + contact->hash_id + "'";
	}

	QSqlQuery readQuery(db);
	readQuery.prepare("SELECT contact_hash_id, message, incomming, msg_read, timestamp FROM message_history WHERE "
					  + contactsQueryPart + " AND timestamp>=:timestamp ORDER BY timestamp ASC;");

	readQuery.bindValue(":timestamp", timestamp_encode(earliest));

	QList<Message> event_list = read_history(readQuery, mark_read);
	foreach(Message m, event_list)
		events_i->fire_event(m);
}

void History::refire_latest_events(QList<Contact *> contacts, int count, bool mark_read) {
	QString contactsQueryPart;
	foreach(Contact *contact, contacts) {
		if(contactsQueryPart.size())
			contactsQueryPart += " OR ";
		contactsQueryPart += "contact_hash_id='" + contact->hash_id + "'";
	}

	QSqlQuery readQuery(db);
	readQuery.prepare("SELECT contact_hash_id, message, incomming, msg_read, timestamp FROM message_history WHERE "
					  + contactsQueryPart + " ORDER BY timestamp DESC LIMIT :count;");

	readQuery.bindValue(":count", count);

	QList<Message> event_list = read_history(readQuery, mark_read);
	foreach(Message m, event_list)
		events_i->fire_event(m);
}

void History::mark_as_read(Contact *contact, QDateTime timestamp) {
	mark_as_read(contact, timestamp_encode(timestamp));
}

void History::mark_as_read(Contact *contact, double timestamp) {
	QSqlQuery mrq(db);
	QString queryText = "UPDATE message_history SET msg_read='true' WHERE contact_hash_id='" + contact->hash_id + "'"
		+ " AND timestamp=:timestamp;";
	
	mrq.prepare(queryText);
	mrq.bindValue(":timestamp", timestamp);
	
	if(!mrq.exec())
		qWarning() << "History mark as read failed:" << mrq.lastError().text();
	else
		if(contact->has_property("PendingMsg")) {
			QList<QVariant> times = contact->get_property("PendingMsg").toList();
			int index;
			if((index = times.indexOf(timestamp)) != -1)
				times.removeAt(index);
			if(times.size() == 0) {
				contact->remove_property("PendingMsg");
				ContactChanged cc(contact, this);
				events_i->fire_event(cc);
			} else
				contact->set_property("PendingMsg", times);
		}
}

void History::mark_all_as_read(Contact *contact) {
	QSqlQuery mrq(db);
	QString queryText = "UPDATE message_history SET msg_read='true' WHERE contact_hash_id='" + contact->hash_id + "'"
		+ " AND msg_read='false';";
	
	if(!mrq.exec(queryText))
		qWarning() << "History mark all as read failed:" << mrq.lastError().text();
	else
		if(contact->has_property("PendingMsg")) {
			contact->remove_property("PendingMsg");
			ContactChanged cc(contact, this);
			events_i->fire_event(cc);
		}
}

void History::wipe_history(Contact *contact) {
	QSqlQuery wq(db);
	QString queryText = "DELETE FROM message_history,contact_hash WHERE contact_hash_id='" + contact->hash_id + "';";
	if(!wq.exec(queryText))
		qWarning() << "History wipe failed:" << wq.lastError().text();
	hashMap.remove(contact->hash_id);
}

void History::enable_history(Contact *contact, bool enable) {
	if(enable) contact->remove_property("DisableHistory");
	else contact->set_property("DisableHistory", true);

	ContactChanged cc(contact, this);
	events_i->fire_event(cc);
}

/////////////////////////////

Q_EXPORT_PLUGIN2(history, History)

