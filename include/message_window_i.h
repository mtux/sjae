#ifndef _I_MESSAGE_WINDOW_H
#define _I_MESSAGE_WINDOW_H

#include "plugin_i.h"
#include "contact_info_i.h"

#define INAME_MESSAGE_WINDOW	"MessageWindowInterface"

#define UUID_MSG_WIN		"{0CA32300-AAAA-4ed9-9867-42320DAC7BD7}"

class MessageWinEvent: public EventsI::Event {
public:
	MessageWinEvent(Contact *c, QObject *source = 0): EventsI::Event(UUID_MSG_WIN, source), contact(c), removed(false) {}
	Contact *contact;
	bool removed;
};


class MessageWindowI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_MESSAGE_WINDOW;}

public slots:
	virtual void open_window(Contact *contact) = 0;
};

#endif
