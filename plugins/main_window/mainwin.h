#ifndef MAINWIN_H
#define MAINWIN_H

#include <QMainWindow>
#include "ui_mainwin.h"
#include <icons_i.h>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QPoint>

class MainWin : public QMainWindow
{
	Q_OBJECT

public:
	MainWin(CoreI *core, QWidget *parent = 0);
	~MainWin();

	void add_window(QWidget *w);
	void manage_window_position(QWidget *w);
	void add_submenu(QMenu *menu);
	void restoreHiddenState();

	void set_hide_toolbar(bool hide);
	void set_hide_frame(bool hide);
	void set_tool_window(bool tool);
	void set_transparency(int trans_percent);
	void set_round_corners(bool round);

public slots:
	void toggleHidden();
	void quit();

protected:
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void resizeEvent(QResizeEvent *e);
	void paintEvent(QPaintEvent *e);

	bool eventFilter(QObject *target, QEvent *e);

	CoreI *core_i;
	QPointer<IconsI> icons_i;

	QMenu *winMenu;

	bool closing;
	QAction *sepAction;
	QSystemTrayIcon *systray;

	bool mousePressed;
	QPoint cursorOffset;
	bool hideFrame, toolWindow, roundCorners;
	void updateFlags();

protected slots:
	void systrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
	Ui::MainWinClass ui;
};

#endif // MAINWIN_H
