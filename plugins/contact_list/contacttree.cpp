#include "contacttree.h"
#include <QContextMenuEvent>
#include <QDebug>
#include <QHelpEvent>

ContactTree::ContactTree(QWidget *parent)
	: QTreeView(parent), tip_shown(false)
{
	ui.setupUi(this);

	/*
	QTreeWidgetItem *header = new QTreeWidgetItem();
	setHeaderItem(header);
	header->setHidden(true);
	*/

	setMouseTracking(true);
	setAttribute(Qt::WA_AlwaysShowToolTips, true);
}

ContactTree::~ContactTree()
{

}

bool ContactTree::viewportEvent(QEvent *e) {
	if(e->type() == QEvent::ToolTip) {
		QHelpEvent *he = static_cast<QHelpEvent *>(e);
		if(tip_shown)
			emit hide_tip();
		tip_shown = false;
		QModelIndex i = indexAt(he->pos());
		if(i.isValid()) {
			tip_shown = true;
			emit show_tip(i, QCursor::pos()); // mapToGlobal(he->pos());
		}
	}
	return QTreeView::viewportEvent(e);
}

void ContactTree::mousePressEvent(QMouseEvent *e) {
	if(tip_shown) {
		tip_shown = false;
		emit hide_tip();
	}
	return QTreeView::mousePressEvent(e);
}

void ContactTree::mouseReleaseEvent(QMouseEvent *e) {
	if(tip_shown) {
		tip_shown = false;
		emit hide_tip();
	}
	return QTreeView::mouseReleaseEvent(e);
}

void ContactTree::mouseMoveEvent(QMouseEvent *e) {
	if(tip_shown) {
		tip_shown = false;
		emit hide_tip();
	}
	return QTreeView::mouseMoveEvent(e);
}

