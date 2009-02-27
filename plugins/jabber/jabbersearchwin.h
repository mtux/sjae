#ifndef JABBERSEARCHWIN_H
#define JABBERSEARCHWIN_H

#include <QWidget>
#include "ui_jabbersearchwin.h"

#include <add_contact_i.h>
#include "jabber.h"

class JabberSearchWin : public ProtoSearchWindowI
{
	Q_OBJECT

public:
	JabberSearchWin(JabberProto *jabber_proto, QWidget *parent = 0);
	~JabberSearchWin();

public slots:
	void set_account(const QString &id);

private:
	Ui::JabberSearchWinClass ui;
	JabberProto *proto;
	QString account;

private slots:
	void on_addBtn_clicked();
};

#endif // JABBERSEARCHWIN_H
