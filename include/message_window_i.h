#ifndef _I_MESSAGE_WINDOW_H
#define _I_MESSAGE_WINDOW_H

#include "plugin_i.h"

#define INAME_MESSAGE_WINDOW	"MessageWindowInterface"

class MessageWindowI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_MESSAGE_WINDOW;}

public slots:
	virtual void open_window(const QString &proto, const QString &account_id, const QString &contact_id) = 0;
};

#endif
