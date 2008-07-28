#ifndef _I_ADD_CONTACT_H
#define _I_ADD_CONTACT_H

#include "plugin_i.h"
#include <QString>
#include <QWidget>

#define INAME_ADD_CONTACT	"AddContactInterface"

class ProtoSearchWindowI: public QWidget {
	Q_OBJECT
public:
	ProtoSearchWindowI(QWidget *parent = 0): QWidget(parent) {}
	virtual ~ProtoSearchWindowI() {}
public slots:
	virtual void set_account(const QString &id) = 0;
};

class AddContactI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_ADD_CONTACT;}
public slots:
	virtual void open_search_window() = 0;
};

#endif
