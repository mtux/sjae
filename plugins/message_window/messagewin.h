#ifndef MESSAGEWIN_H
#define MESSAGEWIN_H

#include <QDialog>
#include "ui_messagewin.h"

class MessageWin : public QDialog
{
	Q_OBJECT

public:
	MessageWin(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &nick = "", QWidget *parent = 0);
	~MessageWin();

public slots:
	void msgRecv(const QString &msg);
signals:
	void msgSend(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg);
private:
	Ui::MessageWinClass ui;
	QString proto_name, account_id, contact_id, nick;

private slots:
	void on_btnSend_clicked();
};

#endif // MESSAGEWINDOW_H
