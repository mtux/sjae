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
	
	ContactTree *tree() {return ui.treeWidget;}
	QMenu *contact_menu() {return &contactMenu;}

protected slots:
	void menuReq(const QPoint &p);
signals:
	void aboutToShowMenu(QTreeWidgetItem *i);

private:
	QMenu contactMenu;
	Ui::CListWinClass ui;
};

#endif // CLISTWIN_H
