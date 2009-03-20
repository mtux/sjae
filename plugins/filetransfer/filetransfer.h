#ifndef __FILETRANSFER_H
#define __FILETRANSFER_H

#include <plugin_i.h>		
#include <filetransfer_i.h>				
#include <options_i.h> 				
#include "filetransferoptions.h" 		
#include "ftprogressdialog.h"
#include "ftid.h"

class FileTransfer: public FileTransferI, EventsI::EventListener{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	FileTransfer();
	~FileTransfer();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	bool event_fired(EventsI::Event &e);

protected slots:				
	void options_applied();		
	void cancelled(const FTId &ftid);
protected:
	CoreI *core_i;
	FileTransferOptions *opt;
	QPointer<EventsI> events_i;

	QMap<FTId, FTProgressDialog *> dialogs;
};

#endif // FILETRANSFER

