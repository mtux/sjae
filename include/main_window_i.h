#ifndef _I_MAINWINDOW_H
#define _I_MAINWINDOW_H

#include "plugin_i.h"
#include <QString>
#include <QStatusBar>
#include <QMenu>

#define INAME_MAINWINDOW	"MainWindowInterface"

class MainWindowI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_MAINWINDOW;}

	virtual void set_central_widget(QWidget *w) = 0;
	virtual void set_status_bar(QStatusBar *sb) = 0;
	virtual void add_window(QWidget *w) = 0;
	virtual void manage_window_position(QWidget *w) = 0;
};

#endif
