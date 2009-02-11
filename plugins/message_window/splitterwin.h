#ifndef SPLITTERWIN_H
#define SPLITTERWIN_H

#include <QSplitter>
#include "ui_splitterwin.h"
#include <accounts_i.h>
#include <events_i.h>

class SplitterWin : public QSplitter { 
	Q_OBJECT

public:
	SplitterWin(Contact *contact, EventsI *events_i, QWidget *parent = 0);
	~SplitterWin();
public slots:
	void msgRecv(const QString &msg);
	void msgSend(const QString &msg);

	void setLogStyleSheet(const QString &styleSheet);

private:
	Ui::SplitterWinClass ui;
	Contact *contact;
	QString timestamp();
	QPointer<EventsI> events_i;
	bool showDate, showTime, showNick;
};

#endif // SPLITTERWIN_H
