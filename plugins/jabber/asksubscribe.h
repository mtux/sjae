#ifndef ASKSUBSCRIBE_H
#define ASKSUBSCRIBE_H

#include <QDialog>
#include "ui_asksubscribe.h"

class AskSubscribe : public QDialog
{
	Q_OBJECT

public:
	AskSubscribe(QWidget *parent = 0);
	~AskSubscribe();
public slots:
	void addUser(const QString &jid, const QString &account_id);
protected slots:
	void emitGrantsAndClear();
	void clearList();
signals:
	void grant(const QString &jid, const QString &account_id);
	void deny(const QString &jid, const QString &account_id);
private:
	Ui::AskSubscribeClass ui;
private slots:
	void on_btnInvertSelection_clicked();
	void on_btnSelectAll_clicked();
};

#endif // ASKSUBSCRIBE_H
