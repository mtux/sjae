#ifndef MAINWIN_H
#define MAINWIN_H

#include <QMainWindow>
#include "ui_mainwin.h"
#include <icons_i.h>
#include <QPointer>
#include <QSystemTrayIcon>

class MainWin : public QMainWindow
{
	Q_OBJECT

public:
	MainWin(CoreI *core, QWidget *parent = 0);
	~MainWin();

	void add_window(QWidget *w);
	void add_submenu(QMenu *menu);
	void restoreHiddenState();
public slots:
	void toggleHidden();

protected:
	bool eventFilter(QObject *target, QEvent *e);

	CoreI *core_i;
	QPointer<IconsI> icons_i;

	QMenu *winMenu;

	void hideEvent(QHideEvent *e);
	void showEvent(QShowEvent *e);
	void closeEvent(QCloseEvent *e);

	bool closing;
	QAction *sepAction;
	QSystemTrayIcon *systray;

protected slots:
	void systrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
	Ui::MainWinClass ui;
};

#endif // MAINWIN_H
