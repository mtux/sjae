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

signals:
	void msgSend(const QString &msg);
private:
	Ui::MessageWinClass ui;

	bool okToSend(QString msg);
private slots:
	void on_btnSend_clicked();
};

#endif // MESSAGEWINDOW_H
