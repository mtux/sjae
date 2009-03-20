#ifndef FTACCEPTDIALOG_H
#define FTACCEPTDIALOG_H

#include <QtGui/QDialog>
#include "ftid.h"

namespace Ui {
    class FTAcceptDialog;
}

class FTAcceptDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(FTAcceptDialog)
public:
	explicit FTAcceptDialog(const FTId &ftid, const QString &filename, int bytes, QWidget *parent = 0);
    virtual ~FTAcceptDialog();

signals:
	void accepted(const FTId &ftid, const QString &newFileName, int size);
	void rejected(const FTId &ftid);

protected:
    virtual void changeEvent(QEvent *e);

private:
	Ui::FTAcceptDialog *m_ui;
	FTId ftid;
	int sizeBytes;

private slots:
	void on_btnBrowse_clicked();

	void emitAccepted();
	void emitRejected();
};

#endif // FTACCEPTDIALOG_H
