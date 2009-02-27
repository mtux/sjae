#ifndef MAINWIN_H
#define MAINWIN_H

#include <QMainWindow>
#include "ui_mainwin.h"
#include <icons_i.h>
#include <QPointer>

class MainWin : public QMainWindow
{
	Q_OBJECT

public:
	MainWin(CoreI *core, QWidget *parent = 0);
	~MainWin();

	void add_window(QWidget *w);
protected:
	bool eventFilter(QObject *target, QEvent *event);

	CoreI *core_i;
	QPointer<IconsI> icons_i;

	QMenu *winMenu;

private:
	Ui::MainWinClass ui;
};

#endif // MAINWIN_H
