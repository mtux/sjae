#ifndef SEARCHWINDOW_H
#define SEARCHWINDOW_H

#include <QWidget>
#include "ui_searchwindow.h"

#include <add_contact_i.h>
#include <QMap>
#include <QStringList>

class SearchWindow : public QWidget
{
	Q_OBJECT

public:
	SearchWindow(QWidget *parent = 0);
	~SearchWindow();

	void add_search_window(const QString &proto_name, ProtoSearchWindowI *search_window);
	void add_account(const QString &proto_name, const QString &id);
	void remove_account(const QString &proto_name, const QString &id);
protected slots:
	void select_protocol(const QString &proto);
	void select_account(const QString &account);
private:
	Ui::SearchWindowClass ui;

	class ProtoData {
	public:
		QStringList accounts;
		ProtoSearchWindowI *search_win;
	};

	QMap<QString, ProtoData> accounts;
};

#endif // SEARCHWINDOW_H
