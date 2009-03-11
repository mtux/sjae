#include "filetransfer.h"
#include <QtPlugin>

PluginInfo info = {
	0x600,
	"FileTransfer",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"FileTransfer",
	0x00000001
};

FileTransfer::FileTransfer()
{

}

FileTransfer::~FileTransfer()
{

}

bool FileTransfer::load(CoreI *core) {
	core_i = core;
	return true;
}

bool FileTransfer::modules_loaded() {
	OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);		
	if(options_i) {																
		opt = new FileTransferOptions();											
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));			
		options_i->add_page("File Transfer", opt);
	}																			
	return true;
}

bool FileTransfer::pre_shutdown() {
	return true;
}

bool FileTransfer::unload() {
	return true;
}

const PluginInfo &FileTransfer::get_plugin_info() {
	return info;
}

/////////////////////////////

void FileTransfer::options_applied() {												
}																				

/////////////////////////////

Q_EXPORT_PLUGIN2(filetransfer, FileTransfer)

