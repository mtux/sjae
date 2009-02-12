#ifndef SPLITTERWIN_H
#define SPLITTERWIN_H

#include <QSplitter>
#include "ui_splitterwin.h"
#include <accounts_i.h>
#include <events_i.h>
#include <QList>

class SplitterWin : public QSplitter { 
	Q_OBJECT

public:
	SplitterWin(Contact *contact, EventsI *events_i, QWidget *parent = 0);
	~SplitterWin();
public slots:
	void msgRecv(const QString &msg, QDateTime &time);
	void msgSend(const QString &msg);

	void setLogStyleSheet(const QString &styleSheet);

protected:
	class MessageData {
	public:
		MessageData(bool in, const QString &msg): incomming(in), message(msg) {}
		bool incomming;
		QString message;
	};

	void update_log();
	QString getNick(Contact *contact);
	QString getContent();

private:
	Ui::SplitterWinClass ui;
	Contact *contact;
	QString timestamp(QDateTime &dt);
	QPointer<EventsI> events_i;
	bool showDate, showTime, showNick;

	QList<MessageData> content;
	QString style;
};

#endif // SPLITTERWIN_H
