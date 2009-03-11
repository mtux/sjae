#ifndef __FILETRANSFER_H
#define __FILETRANSFER_H

#include <plugin_i.h>		
#include <filetransfer_i.h>				
#include <options_i.h> 				
#include "filetransferoptions.h" 		

class FileTransfer: public FileTransferI 	
{
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

protected slots:				
	void options_applied();		
	
protected:
	CoreI *core_i;
	FileTransferOptions *opt;		
};

#endif // FILETRANSFER

