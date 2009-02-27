#ifndef CONTACTTREE_H
#define CONTACTTREE_H

#include <QTreeView>
#include "ui_contacttree.h"

class ContactTree : public QTreeView
{
	Q_OBJECT

public:
	ContactTree(QWidget *parent = 0);
	~ContactTree();

signals:
	void show_tip(const QModelIndex &index, const QPoint &p);
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
