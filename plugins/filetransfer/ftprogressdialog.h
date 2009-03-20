#ifndef FTPROGRESSDIALOG_H
#define FTPROGRESSDIALOG_H

#include <QtGui/QDialog>
#include <contact_info_i.h>
#include "ftid.h"

namespace Ui {
    class FTProgressDialog;
}


class FTProgressDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(FTProgressDialog)
public:
	typedef enum {ST_ACCEPTED, ST_CANCELLED, ST_INPROGRESS, ST_COMPLETED} State;

	explicit FTProgressDialog(const FTId &ftid, const QString &filename, int bytes, QWidget *parent = 0);
    virtual ~FTProgressDialog();

	void setState(State s);
	State getState() {return state;}
	void setProgress(int prog);
	bool getIncoming() {return ftid.incoming;}

signals:
	void cancelled(const FTId &ftid);

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::FTProgressDialog *m_ui;
	FTId ftid;
	int sizeBytes;
	State state;

private slots:
	void on_btnDone_clicked();
};

#endif // FTPROGRESSDIALOG_H
