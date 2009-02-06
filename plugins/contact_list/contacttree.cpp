#include "contacttree.h"
#include <QContextMenuEvent>
#include <QDebug>
#include <QHelpEvent>

ContactTree::ContactTree(QWidget *parent)
	: QTreeWidget(parent), tip_shown(false)
{
	ui.setupUi(this);
	setColumnCount(1);

	/*
	QTreeWidgetItem *header = new QTreeWidgetItem();
	setHeaderItem(header);
	header->setHidden(true);
	*/

	setMouseTracking(true);
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
		QTreeWidgetItem *i = itemAt(he->pos());
		if(i) {
			tip_shown = true;
			emit show_tip(i, QCursor::pos()); // mapToGlobal(he->pos());
		}
	}
	return QTreeWidget::viewportEvent(e);
}

void ContactTree::mousePressEvent(QMouseEvent *e) {
	if(tip_shown) {
		tip_shown = false;
		emit hide_tip();
	}
	return QTreeWidget::mousePressEvent(e);
}

void ContactTree::mouseReleaseEvent(QMouseEvent *e) {
	if(tip_shown) {
		tip_shown = false;
		emit hide_tip();
	}
	return QTreeWidget::mouseReleaseEvent(e);
}

void ContactTree::mouseMoveEvent(QMouseEvent *e) {
	if(tip_shown) {
		tip_shown = false;
		emit hide_tip();
	}
	return QTreeWidget::mouseMoveEvent(e);
}

