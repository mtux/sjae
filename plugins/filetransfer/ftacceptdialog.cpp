#include "ftacceptdialog.h"
#include "ui_ftacceptdialog.h"
#include <QFileDialog>
#include <QDesktopServices>

FTAcceptDialog::FTAcceptDialog(const FTId &id, const QString &filename, int bytes, QWidget *parent) :
    QDialog(parent),
	m_ui(new Ui::FTAcceptDialog), ftid(id), sizeBytes(bytes)
{
    m_ui->setupUi(this);

	m_ui->lblContact->setText(ftid.contact->nick() + "(" + ftid.contact->contact_id + ")");
	m_ui->edFileName->setText(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/SajeDownloads/" + filename);
	if(bytes < 1024)
		m_ui->lblSize->setText(QString("%1 b").arg(bytes));
	else if(bytes < 1024 * 1024)
		m_ui->lblSize->setText(QString("%1 kb").arg(bytes/1024.0, 0, 'f', 1));
	else
		m_ui->lblSize->setText(QString("%1 mb").arg(bytes/(1024.0 * 1024), 0, 'f', 2));

	setAttribute(Qt::WA_DeleteOnClose, true);

	connect(this, SIGNAL(accepted()), this, SLOT(emitAccepted()));
	connect(this, SIGNAL(rejected()), this, SLOT(emitRejected()));
}

FTAcceptDialog::~FTAcceptDialog() {
    delete m_ui;
}

void FTAcceptDialog::changeEvent(QEvent *e) {
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void FTAcceptDialog::on_btnBrowse_clicked() {
	QString fn = QFileDialog::getOpenFileName(this, "Save File...", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/SajeDownloads");
	if(!fn.isEmpty())
		m_ui->edFileName->setText(fn);
}

void FTAcceptDialog::emitAccepted() {
	emit accepted(ftid, m_ui->edFileName->text(), sizeBytes);
}

void FTAcceptDialog::emitRejected() {
	emit rejected(ftid);
}

