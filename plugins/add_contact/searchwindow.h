#ifndef SEARCHWINDOW_H
#define SEARCHWINDOW_H

#include <QWidget>
#include "ui_searchwindow.h"

#include <add_contact_i.h>
#include <accounts_i.h>
#include <QMap>
#include <QStringList>

class SearchWindow : public QWidget
{
	Q_OBJECT

public:
	SearchWindow(QWidget *parent = 0);
	~SearchWindow();

	void add_search_window(const QString &proto_name, ProtoSearchWindowI *search_window);
	void add_account(Account *acc);
	void remove_account(Account *acc);

	bool has_account(Account *acc);

protected slots:
	void select_protocol(const QString &proto);
	void select_account(int i);
private:
	Ui::SearchWindowClass ui;

	class ProtoData {
	public:
		QList<Account *> accounts;
		ProtoSearchWindowI *search_win;
	};

	QMap<QString, ProtoData> accounts;
};

#endif // SEARCHWINDOW_H
