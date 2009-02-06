#ifndef SPLITTERWIN_H
#define SPLITTERWIN_H

#include <QSplitter>
#include "ui_splitterwin.h"

class SplitterWin : public QSplitter
{
	Q_OBJECT

public:
	SplitterWin(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &contact_nick = "", const QString &local_nick = "", QWidget *parent = 0);
	~SplitterWin();
public slots:
	void msgRecv(const QString &msg);
	void msgSend(const QString &msg);

	void setLogStyleSheet(const QString &styleSheet);
signals:
	void msgSend(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg);

private:
	Ui::SplitterWinClass ui;
	QString proto_name, account_id, contact_id, contact_nick, my_nick;
	QString timestamp();
	bool showDate, showTime, showNick;
};

#endif // SPLITTERWIN_H
