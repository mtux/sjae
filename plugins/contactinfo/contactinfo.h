#ifndef __CONTACTINFO_H
#define __CONTACTINFO_H

#include <contact_info_i.h>
#include <events_i.h>
#include <QSqlDatabase>
#include <QPointer>
#include <QTimer>
#include <QMutex>

class ContactInfo: public ContactInfoI {
	Q_OBJECT
public:
	ContactInfo();
	~ContactInfo();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	Contact *get_contact(Account *acc, const QString &contact_id);
	Contact *get_contact(const QString &contact_hash_id);
	bool delete_contact(Contact *contact);

	bool event_fired(EventsI::Event &e);

protected:
	CoreI *core_i;
	QPointer<EventsI> events_i;
	QPointer<AccountsI> accounts_i;

	QMutex dataMutex;
	QMap<Account *, QMap<QString, Contact *> > contacts;
	QMap<QString, Contact *> hashMap;
	QSqlQuery *get_props, *replace_prop, *delete_prop, *delete_props;

	QSqlDatabase db;

	class DBWrite {
	public:
		Contact *contact;
		QStringList settings;
	};

};

#endif // CONTACTINFO

