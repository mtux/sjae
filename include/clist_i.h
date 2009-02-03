#ifndef _I_CLIST_H
#define _I_CLIST_H

#include "plugin_i.h"
#include "global_status.h"
#include <QString>
#include <QTreeWidgetItem>
#include <QAction>

#define INAME_CLIST	"CListInterface"

// tree widget item types
#define TWIT_GROUP				0x10
#define TWIT_CONTACT			0x20

class CListI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	const QString get_interface_name() const {return INAME_CLIST;}

	virtual QTreeWidgetItem *add_contact(const QString &proto_name, const QString &account_id, const QString &id, const QString &label, GlobalStatus gs, const QString &group = "") = 0;
	virtual QAction *add_contact_action(const QString &proto_name, const QString &account_id, const QString &label, const QString &icon = "") = 0;

	virtual bool get_hide_offline() {return hide_offline;}
public slots:
	virtual void set_group_delimiter(const QString &proto_name, const QString &account_id, const QString &delim) = 0;
	virtual void remove_contact(const QString &proto_name, const QString &account_id, const QString &id) = 0;
	virtual void set_label(const QString &proto_name, const QString &account_id, const QString &id, const QString &label) = 0;
	virtual void set_group(const QString &proto_name, const QString &account_id, const QString &id, const QString &group) = 0;
	virtual void set_status(const QString &proto_name, const QString &account_id, const QString &id, GlobalStatus gs) = 0;
	//virtual void set_hidden(const QString &proto_name, const QString &account_id, const QString &id, bool hide) = 0;

	virtual void set_hide_offline(bool hide) {hide_offline = hide;}

signals:
	void contact_clicked(const QString &proto_name, const QString &account_id, const QString &id);
	void contact_dbl_clicked(const QString &proto_name, const QString &account_id, const QString &id);
	void show_tip(const QString &proto_name, const QString &account_id, const QString &id, const QPoint &p);
	void hide_tip();
	//void label_changed(const QString &proto_name, const QString &account_id, const QString &id, const QString &label);
	void aboutToShowContactMenu(const QString &proto_name, const QString &account_id, const QString &id);
	void aboutToShowGroupMenu(const QString &proto_name, const QString &account_id, const QString &full_gn);

protected:
	bool hide_offline;
};

#endif
