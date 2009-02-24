#ifndef CLISTWIN_H
#define CLISTWIN_H

#include <QWidget>
#include "ui_clistwin.h"
#include <QMenu>

class CListWin : public QWidget
{
	Q_OBJECT

public:
	CListWin(QWidget *parent = 0);
	~CListWin();
	
	QTreeView *tree() {return ui.treeView;}
	QMenu *contact_menu() {return &contactMenu;}
	QMenu *group_menu() {return &groupMenu;}

protected slots:
	void menuReq(const QPoint &p);
signals:
	void showMenu(const QPoint &p, const QModelIndex &index);

private:
	QMenu contactMenu;
	QMenu groupMenu;
	Ui::CListWinClass ui;
};

#endif // CLISTWIN_H
