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
	void closeManual();
signals:
	void closed(int i);

protected slots:
	void timeout();
	void mouseClose();

protected:
	void closeEvent(QCloseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	bool eventFilter(QObject *obj, QEvent *e);
private:
	Ui::PopupWinClass ui;
	PopupI::PopupListener *listener;
	int id;
	QTimer closeTimer;
	bool round_corners;

	PopupI::PopupDoneType mouseCloseReason;
};

#endif // POPUPWIN_H
