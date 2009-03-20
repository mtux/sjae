#include "filetransfer.h"
#include <QtPlugin>
#include <QMessageBox>
#include "ftacceptdialog.h"

#include <QDesktopServices>
#include <QDir>
#include <QDebug>

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

	QDir saveDir(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/SajeDownloads");
	if(!saveDir.exists() && !saveDir.mkdir("."))
		qWarning() << "Failed to create directory:" << saveDir.absolutePath();

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

void FileTransfer::accepted(const FTId &ftid, const QString &newFileName, int size) {
	FileTransferUserEvent r(ftid.contact, newFileName, size, ftid.id, this);
	r.type = EventsI::ET_OUTGOING;
	r.ftType = FileTransferUserEvent::FT_ACCEPT;
	r.incoming = ftid.incoming;
	events_i->fire_event(r);

	dialogs[ftid] = new FTProgressDialog(ftid, newFileName, size);
	connect(dialogs[ftid], SIGNAL(cancelled(const FTId &)), this, SLOT(cancelled(const FTId &)));
	dialogs[ftid]->setState(FTProgressDialog::ST_ACCEPTED);
	dialogs[ftid]->show();
}

void FileTransfer::rejected(const FTId &ftid) {
	FileTransferUserEvent r(ftid.contact, "", 0, ftid.id, this);
	r.type = EventsI::ET_OUTGOING;
	r.ftType = FileTransferUserEvent::FT_CANCEL;
	r.incoming = ftid.incoming;
	events_i->fire_event(r);
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
				FTAcceptDialog *fta = new FTAcceptDialog(ftid, ftue.fileName, ftue.sizeBytes, 0);
				connect(fta, SIGNAL(accepted(const FTId &, const QString &, int)), this, SLOT(accepted(const FTId &, const QString &, int)));
				connect(fta, SIGNAL(rejected(const FTId &)), this, SLOT(rejected(const FTId &)));
				fta->show();
			} else {
				dialogs[ftid] = new FTProgressDialog(ftid, ftue.fileName, ftue.sizeBytes);
				connect(dialogs[ftid], SIGNAL(cancelled(const FTId &)), this, SLOT(cancelled(const FTId &)));
				dialogs[ftid]->show();
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
			r.incoming = ftid.incoming;
			events_i->fire_event(r);
			qDebug() << "sending cancel event";
		}
	}
}

/////////////////////////////

Q_EXPORT_PLUGIN2(filetransfer, FileTransfer)

