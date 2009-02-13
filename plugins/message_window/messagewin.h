#ifndef MESSAGEWIN_H
#define MESSAGEWIN_H

#include <QDialog>
#include "ui_messagewin.h"

class MessageWin : public QWidget
{
	Q_OBJECT

public:
	MessageWin(QWidget *parent = 0);
	~MessageWin();

	bool okToSend();
signals:
	void msgSend(const QString &msg);
	void textChanged();

private:
	Ui::MessageWinClass ui;

private slots:
	void on_btnSend_clicked();
};

#endif // MESSAGEWINDOW_H
