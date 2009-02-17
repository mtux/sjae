#ifndef POPUPWIN_H
#define POPUPWIN_H

#include <QWidget>
#include "ui_popupwin.h"
#include <popup_i.h>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QTimer>

class PopupWin : public QWidget
{
	Q_OBJECT

public:
	PopupWin(const PopupI::PopupClass &c, int id, bool round_corners = true, QWidget *parent = 0);
	~PopupWin();

	void setIcon(const QIcon &icon);
	void setContent(const QString &title, const QString &text);

	int getId() {return id;}
signals:
	void closed(int i);

protected slots:
	void timeout();
protected:
	void closeEvent(QCloseEvent *e);
	void mousePressEvent(QMouseEvent *e);

private:
	Ui::PopupWinClass ui;
	PopupI::PopupListener *listener;
	int id;
	QTimer closeTimer;
};

#endif // POPUPWIN_H
