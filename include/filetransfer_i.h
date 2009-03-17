#ifndef _I_FILETRANSFER_H
#define _I_FILETRANSFER_H

#include "plugin_i.h"
#include "events_i.h"
#include "contact_info_i.h"

#define INAME_FILETRANSFER	"FileTransferInterface"

#define UUID_FT			"{44B484B8-4B1D-40a0-A142-8A34FBF2D771}"

class FileTransferReq: public EventsI::Event {
public:
	FileTransferReq(Contact *c, QObject *source = 0): EventsI::Event(UUID_FT, source), contact(c) {}
	Contact *contact;
	QString fileName;
	int size, id;
};

class FileTransferI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	const QString get_interface_name() const {return INAME_FILETRANSFER;}
};

#endif
