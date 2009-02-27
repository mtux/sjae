#include "history.h"
#include <QtPlugin>
#include <QDebug>
#include <QSqlError>

#define DB_FILE_NAME		"message_history.db"

double timestamp_encode(const QDateTime &d) {
	return d.toTime_t() + d.time().msec() / 1000.0;
}

QDateTime timestamp_decode(const double val) {
	uint secs = (uint)val;
	int msecs = (val - secs) * 1000 + 0.5;
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

	events_i->add_event_listener(this, UUID_MSG);

	db = QSqlDatabase::addDatabase("QSQLITE", "History");
	db.setDatabaseName(core_i->get_config_dir() + "/" + DB_FILE_NAME);
    if(!db.open()) return false;

	QSqlQuery q(db);
	if(!q.exec("CREATE TABLE messages ("
		"  protocol varchar(256),"
		"  account varchar(256),"
		"  contact_id varchar(256),"
		"  timestamp number,"
		"  incomming boolean,"
		"  msg_read boolean,"
		"  message text);"))
	{
		qWarning() << "History db error:" << q.lastError().text();
	}
	
	return true;
}

bool History::modules_loaded() {
	QSqlQuery unread(db);
	if(!unread.exec("SELECT protocol, account, contact_id, message, incomming, timestamp FROM messages WHERE msg_read='false';"))
		qWarning() << "History read unread failed:" << unread.lastError().text();

	while(unread.next()) {
		Account *account = accounts_i->account_info(unread.value(0).toString(), unread.value(1).toString());
		if(account) {
			Contact *contact = contact_info_i->get_contact(account, unread.value(2).toString());
			ContactChanged cc(contact, this);
			events_i->fire_event(cc);
			Message m(contact, unread.value(3).toString(), unread.value(4).toBool(), 0, this);
			m.timestamp = timestamp_decode(unread.value(5).toDouble());
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
			if(m.contact->has_property("DisableHistory"))
				return true;

			writeQuery = new QSqlQuery(db);
			writeQuery->prepare("INSERT INTO messages VALUES(?, ?, ?, ?, ?, ?, ?);");

			writeQuery->addBindValue(m.contact->account->proto->name());
			writeQuery->addBindValue(m.contact->account->account_id);
			writeQuery->addBindValue(m.contact->contact_id);
			writeQuery->addBindValue(t);
			writeQuery->addBindValue(m.type == EventsI::ET_INCOMMING);
			writeQuery->addBindValue(m.read);
			writeQuery->addBindValue(m.text);

			if(!writeQuery->exec()) {
				qWarning() << "History write failed:" << writeQuery->lastError().text();
			}

			delete writeQuery;
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

QList<Message> History::read_history(Contact *contact, QSqlQuery &query, bool mark_read) {
	if(!query.exec()) {
		qWarning() << "History read failed:" << query.lastError().text();
	}

	QList<Message> ret;
	double t;
	while(query.next()) {
		Message m(contact, query.value(0).toString(), query.value(1).toBool(), 0, this);
		t = query.value(2).toDouble();
		m.timestamp = timestamp_decode(t);
		if(mark_read) {
			mark_as_read(contact, t);
			m.read = true;
		}
		ret << m;
	}

	return ret;
}

QList<Message> History::get_latest_events(Contact *contact, QDateTime earliest, bool mark_read) {

	QSqlQuery readQuery(db);
	readQuery.prepare("SELECT message, incomming, timestamp FROM messages WHERE protocol=:proto AND account=:account AND contact_id=:contact_id AND timestamp>=:timestamp ORDER BY timestamp ASC;");

	readQuery.bindValue(":proto", contact->account->proto->name());
	readQuery.bindValue(":account", contact->account->account_id);
	readQuery.bindValue(":contact_id", contact->contact_id);
	readQuery.bindValue(":timestamp", earliest.toTime_t() + earliest.time().msec() / 1000.0);

	return read_history(contact, readQuery, mark_read);

}

QList<Message> History::get_latest_events(Contact *contact, int count, bool mark_read) {

	QSqlQuery readQuery(db);
	readQuery.prepare("SELECT message, incomming, timestamp FROM messages WHERE protocol=:proto AND account=:account AND contact_id=:contact_id ORDER BY timestamp ASC LIMIT :count;");

	readQuery.bindValue(":proto", contact->account->proto->name());
	readQuery.bindValue(":account", contact->account->account_id);
	readQuery.bindValue(":contact_id", contact->contact_id);
	readQuery.bindValue(":count", count);

	return read_history(contact, readQuery, mark_read);
}

QList<Message> History::get_latest_events(QList<Contact *> contacts, QDateTime earliest, bool mark_read) {
	QList<Message> ret;
	foreach(Contact *contact, contacts) {
		ret += get_latest_events(contact, earliest, mark_read);
	}
	qSort(ret);
	return ret;
}

QList<Message> History::get_latest_events(QList<Contact *> contacts, int count, bool mark_read) {
	/*
	QString queryString = "SELECT message, incomming, timestamp FROM messages WHERE ";
	bool first = true;
	foreach(Contact *contact, contacts) {
		if(!first) queryString += " OR ";
		queryString += "(protocol='" + contact->account->proto->name() + "'";
		queryString += " AND account='" + contact->account->account_id + "'";
		queryString += " AND contact_id='" + contact->contact_id + "')";
		first = false;
	}
	queryString += " ORDER BY timestamp DESC LIMIT :count;";

	QSqlQuery readQuery(db);
	readQuery.prepare(queryString);
	readQuery.bindValue(":count", count);

	return read_history(0, readQuery, mark_read);
	*/
	QList<Message> ret;
	foreach(Contact *contact, contacts) {
		ret += get_latest_events(contact, count, mark_read);
	}
	qSort(ret);
	return ret;
}

void History::mark_as_read(Contact *contact, QDateTime timestamp) {
	mark_as_read(contact, timestamp_encode(timestamp));
}

void History::mark_as_read(Contact *contact, double timestamp) {
	QSqlQuery mrq(db);
	QString queryText = "UPDATE messages SET msg_read='true' WHERE protocol='" + contact->account->proto->name() + "'"
		+ " AND account='" + contact->account->account_id + "'"
		+ " AND contact_id='" + contact->contact_id + "'" + 
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
	QString queryText = "UPDATE messages SET msg_read='true' WHERE protocol='" + contact->account->proto->name() + "'"
		+ " AND account='" + contact->account->account_id + "'"
		+ " AND contact_id='" + contact->contact_id + "'" + 
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
	QString queryText = "DELETE FROM messages WHERE protocol='" + contact->account->proto->name() + "'"
		+ " AND account='" + contact->account->account_id + "'"
		+ " AND contact_id='" + contact->contact_id + "';";
	if(!wq.exec(queryText))
		qWarning() << "History wipe failed:" << wq.lastError().text();
}

void History::enable_history(Contact *contact, bool enable) {
	if(enable) contact->remove_property("DisableHistory");
	else contact->set_property("DisableHistory", true);

	ContactChanged cc(contact, this);
	events_i->fire_event(cc);
}

/////////////////////////////

Q_EXPORT_PLUGIN2(history, History)

