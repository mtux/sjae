#include "filetransfer.h"
#include <QtPlugin>
#include <QMessageBox>

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
	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;

	events_i->add_event_listener(this, UUID_FT);
	events_i->add_event_listener(this, UUID_FT_USER);
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
	events_i->remove_event_listener(this, UUID_FT);
	events_i->remove_event_listener(this, UUID_FT_USER);
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

bool FileTransfer::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_FT_USER) {
		FileTransferUserEvent &ftue = (FileTransferUserEvent &)e;
		FTId ftid;
		ftid.contact = ftue.contact;
		ftid.id = ftue.id;
		ftid.incoming = ftue.incoming;

		if(ftue.ftType == FileTransferUserEvent::FT_REQUEST) {
			if(ftue.type == EventsI::ET_INCOMING) {
				FileTransferUserEvent r(ftue.contact, ftue.fileName, ftue.sizeBytes, ftue.id, ftue.source);
				r.type = EventsI::ET_OUTGOING;

				if(QMessageBox::information(0, tr("Incoming File..."), "File: " + ftue.fileName + "\nFrom: " + ftue.contact->contact_id + "\n\nAccept?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Cancel) {
					r.ftType = FileTransferUserEvent::FT_CANCEL;
					events_i->fire_event(r);
					return true;
				}

				r.ftType = FileTransferUserEvent::FT_ACCEPT;
				events_i->fire_event(r);
			}

			dialogs[ftid] = new FTProgressDialog(ftid, ftue.fileName, ftue.sizeBytes);
			connect(dialogs[ftid], SIGNAL(cancelled(const FTId &)), this, SLOT(cancelled(const FTId &)));
			dialogs[ftid]->show();

			if(ftue.type == EventsI::ET_INCOMING) {
				dialogs[ftid]->setState(FTProgressDialog::ST_ACCEPTED);
			}
		} else if(ftue.ftType == FileTransferUserEvent::FT_CANCEL && dialogs.contains(ftid)) {
			dialogs[ftid]->setState(FTProgressDialog::ST_CANCELLED);
		} else if(ftue.ftType == FileTransferUserEvent::FT_ACCEPT && dialogs.contains(ftid)) {
			ftid.incoming = true;
			dialogs[ftid]->setState(FTProgressDialog::ST_ACCEPTED);
		}
	} else if(e.uuid == UUID_FT) {
		FileTransferProgress &ftp = (FileTransferProgress &)e;
		FTId ftid;
		ftid.contact = ftp.contact;
		ftid.id = ftp.id;
		ftid.incoming = ftp.type == EventsI::ET_INCOMING;
		if(dialogs.contains(ftid)) {
			dialogs[ftid]->setProgress(ftp.progressBytes);
		}
	}
	return true;
}

void FileTransfer::cancelled(const FTId &ftid) {
	if(dialogs.contains(ftid)) {
		FTProgressDialog::State s = dialogs[ftid]->getState();
		if(s == FTProgressDialog::ST_COMPLETED) {
			dialogs[ftid]->close();
			dialogs.remove(ftid);
		} else if(s == FTProgressDialog::ST_CANCELLED) {
			FileTransferUserEvent r(ftid.contact, "", 0, ftid.id, this);
			r.type = EventsI::ET_OUTGOING;
			r.ftType = FileTransferUserEvent::FT_CANCEL;
			events_i->fire_event(r);
		}
	}
}

/////////////////////////////

Q_EXPORT_PLUGIN2(filetransfer, FileTransfer)

