#ifndef CONTACTTREE_H
#define CONTACTTREE_H

#include <QTreeWidget>
#include <QSortFilterProxyModel>
#include "ui_contacttree.h"

class ContactTree : public QTreeWidget
{
	Q_OBJECT

public:
	ContactTree(QWidget *parent = 0);
	~ContactTree();

signals:
	void show_tip(QTreeWidgetItem *i, const QPoint &p);
	void hide_tip();

protected:
	bool viewportEvent(QEvent *e);

	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);

	bool tip_shown;
private:
	Ui::ContactTreeClass ui;
};

#endif // CONTACTTREE_H
