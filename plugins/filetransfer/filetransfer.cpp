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

			dialogs[ftue.source][ftue.id] = new FTProgressDialog(ftue.source, ftue.id, ftue.type == EventsI::ET_INCOMING, ftue.contact, ftue.fileName, ftue.sizeBytes);
			connect(dialogs[ftue.source][ftue.id], SIGNAL(cancelled(QObject *, const QString &, Contact *)), this, SLOT(cancelled(QObject *, const QString &, Contact *)));
			dialogs[ftue.source][ftue.id]->show();

			if(ftue.type == EventsI::ET_INCOMING) {
				dialogs[ftue.source][ftue.id]->setState(FTProgressDialog::ST_ACCEPTED);
			}
		} else if(ftue.ftType == FileTransferUserEvent::FT_CANCEL && dialogs[ftue.source].contains(ftue.id)) {
			dialogs[ftue.source][ftue.id]->setState(FTProgressDialog::ST_CANCELLED);
		} else if(ftue.ftType == FileTransferUserEvent::FT_ACCEPT && dialogs[ftue.source].contains(ftue.id)) {
			dialogs[ftue.source][ftue.id]->setState(FTProgressDialog::ST_ACCEPTED);
		}
	} else if(e.uuid == UUID_FT) {
		FileTransferProgress &ftp = (FileTransferProgress &)e;
		if(dialogs[ftp.source].contains(ftp.id)) {
			dialogs[ftp.source][ftp.id]->setProgress(ftp.progressBytes);
		}
	}
	return true;
}

void FileTransfer::cancelled(QObject *source, const QString &id, Contact *contact) {
	if(dialogs[source].contains(id)) {
		FTProgressDialog::State s = dialogs[source][id]->getState();
		if(s == FTProgressDialog::ST_COMPLETED) {
			dialogs[source][id]->close();
			dialogs[source].remove(id);
		} else if(s == FTProgressDialog::ST_CANCELLED) {
			FileTransferUserEvent r(contact, "", 0, id, source);
			r.type = EventsI::ET_OUTGOING;
			r.ftType = FileTransferUserEvent::FT_CANCEL;
			events_i->fire_event(r);
		}
	}
}

/////////////////////////////

Q_EXPORT_PLUGIN2(filetransfer, FileTransfer)

