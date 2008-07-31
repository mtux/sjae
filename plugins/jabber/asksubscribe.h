#ifndef ASKSUBSCRIBE_H
#define ASKSUBSCRIBE_H

#include <QDialog>
#include "ui_asksubscribe.h"

class AskSubscribe : public QDialog
{
	Q_OBJECT

public:
	AskSubscribe(const QString& jid, QWidget *parent = 0);
	~AskSubscribe();

protected slots:
	void emitGrant();
signals:
	void grant(const QString &jid);
private:
	Ui::AskSubscribeClass ui;
};

#endif // ASKSUBSCRIBE_H
