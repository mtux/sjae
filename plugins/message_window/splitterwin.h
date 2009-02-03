#ifndef SPLITTERWIN_H
#define SPLITTERWIN_H

#include <QSplitter>
#include "ui_splitterwin.h"

class SplitterWin : public QSplitter
{
	Q_OBJECT

public:
	SplitterWin(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &nick = "", QWidget *parent = 0);
	~SplitterWin();
public slots:
	void msgRecv(const QString &msg);
	void msgSend(const QString &msg);
signals:
	void msgSend(const QString &proto_name, const QString &account_id, const QString &contact_id, const QString &msg);

private:
	Ui::SplitterWinClass ui;
	QString proto_name, account_id, contact_id, nick;
};

#endif // SPLITTERWIN_H
