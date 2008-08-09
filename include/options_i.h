#ifndef _I_OPTIONS_H
#define _I_OPTIONS_H

#include "plugin_i.h"
#include <QString>
#include <QWidget>

#define INAME_OPTIONS	"OptionsInterface"

class OptionsPageI: public QWidget {
	Q_OBJECT
public:
	OptionsPageI(QWidget *parent = 0): QWidget(parent) {}
	virtual bool apply() = 0;
	virtual void reset() = 0;
signals:
	void changed(bool valid = true);
};

class OptionsI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_OPTIONS;}

	virtual bool add_page(const QString &category, OptionsPageI *page) = 0;
public slots:
	virtual void show_options() = 0;
};

#endif
