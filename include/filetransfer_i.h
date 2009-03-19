#ifndef _I_FILETRANSFER_H
#define _I_FILETRANSFER_H

#include "plugin_i.h"
#include "events_i.h"
#include "contact_info_i.h"

#define INAME_FILETRANSFER	"FileTransferInterface"

#define UUID_FT				"{44B484B8-4B1D-40a0-A142-8A34FBF2D771}"
#define UUID_FT_USER		"{B89C57C8-3542-4e0e-BEA8-AA79394A76A9}"

class FileTransferProgress: public EventsI::Event {
public:
	FileTransferProgress(Contact *c, QObject *source = 0): EventsI::Event(UUID_FT, source), contact(c) {}
	Contact *contact;
	QString fileName;
	int sizeBytes, id, progressBytes;
};

// to send a file, fire a FileTransferUserEvent with type == ET_OUTGOING && ftType == FT_REQUEST - the contact's protocol
// is responsible for then firing an ET_INCOMMING event with ftType == FT_ACCEPT or FT_CANCEL, and in the case of FT_ACCEPT, to
// follow that up with multiple FT_INCOMMING 'FileTransferProgress' events indicating progress

// if you notice an ET_INCOMMING FileTransferUserEvent with ftType == FT_REQUEST, fire an ET_OUTGOING with ftType == FT_ACCEPT or FT_CANCEL
class FileTransferUserEvent: public EventsI::Event {
public:
	typedef enum {FT_REQUEST, FT_ACCEPT, FT_CANCEL} FTType;

	FileTransferUserEvent(Contact *c, const QString &fn, int size, int id, QObject *source = 0): EventsI::Event(UUID_FT_USER, source), contact(c) {}
	Contact *contact;
	QString fileName;
	int sizeBytes, id;
	FTType ftType;
};

class FileTransferI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:

	const QString get_interface_name() const {return INAME_FILETRANSFER;}
};

#endif
