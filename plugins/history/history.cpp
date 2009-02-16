#include "history.h"
#include <QtPlugin>
#include <QDebug>
#include <QSqlError>

#define DB_FILE_NAME		"message_history.db"

PluginInfo info = {
	0x200,
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

	events_i->add_event_listener(this, UUID_MSG);

	db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(core_i->get_config_dir() + "/" + DB_FILE_NAME);
    if(!db.open()) return false;

	QSqlQuery q(db);
	if(!q.exec("CREATE TABLE messages ("
		"  protocol varchar(256),"
		"  account varchar(256),"
		"  contact_id varchar(256),"
		"  timestamp number,"
		"  incomming boolean,"
		"  message text);"))
	{
		qWarning() << "db error." << q.lastError().text();
	}
	
	return true;
}

bool History::modules_loaded() {
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
		writeQuery = new QSqlQuery(db);
		writeQuery->prepare("INSERT INTO messages VALUES(?, ?, ?, ?, ?, ?);");

		writeQuery->addBindValue(m.contact->account->proto->name());
		writeQuery->addBindValue(m.contact->account->account_id);
		writeQuery->addBindValue(m.contact->contact_id);
		writeQuery->addBindValue(m.timestamp.toTime_t());
		writeQuery->addBindValue(m.data.incomming);
		writeQuery->addBindValue(m.data.message);

		if(!writeQuery->exec()) {
			qWarning() << "History write failed:" << writeQuery->lastError().text();
		}

		delete writeQuery;
	}
	return true;
}

QList<Message> History::get_latest_events(Contact *contact, QDateTime earliest) {
	QList<Message> ret;

	readQueryTime = new QSqlQuery(db);
	readQueryTime->prepare("SELECT message, incomming, timestamp FROM messages WHERE protocol=:proto AND account=:account AND contact_id=:contact_id AND timestamp>=:timestamp;");

	readQueryTime->bindValue(":proto", contact->account->proto->name());
	readQueryTime->bindValue(":account", contact->account->account_id);
	readQueryTime->bindValue(":contact_id", contact->contact_id);
	readQueryTime->bindValue(":timestamp", earliest.toTime_t());

	if(!readQueryTime->exec()) {
		qWarning() << "History read failed:" << readQueryTime->lastError().text();
	}

	while(readQueryTime->next()) {
		Message m(readQueryTime->value(0).toString(), readQueryTime->value(1).toBool(), 0, contact, this);
		m.timestamp = QDateTime::fromTime_t(readQueryTime->value(2).toUInt());
		ret << m;
	}

	delete readQueryTime;

	return ret;
}

QList<Message> History::get_latest_events(Contact *contact, int count) {
	QList<Message> ret;

	readQueryCount = new QSqlQuery(db);
	readQueryCount->prepare("SELECT message, incomming, timestamp FROM messages WHERE protocol=:proto AND account=:account AND contact_id=:contact_id ORDER BY timestamp DESC LIMIT :count;");

	readQueryCount->bindValue(":proto", contact->account->proto->name());
	readQueryCount->bindValue(":account", contact->account->account_id);
	readQueryCount->bindValue(":contact_id", contact->contact_id);
	readQueryCount->bindValue(":count", count);

	if(!readQueryCount->exec()) {
		qWarning() << "History read failed:" << readQueryCount->lastError().text();
	}

	while(readQueryCount->next()) {
		Message m(readQueryCount->value(0).toString(), readQueryCount->value(1).toBool(), 0, contact, this);
		m.timestamp = QDateTime::fromTime_t(readQueryCount->value(2).toUInt());
		ret.prepend(m);
	}

	delete readQueryCount;

	return ret;
}

/////////////////////////////

Q_EXPORT_PLUGIN2(history, History)

