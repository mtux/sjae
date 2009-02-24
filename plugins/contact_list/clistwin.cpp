#include "clistwin.h"
#include <QDebug>

CListWin::CListWin(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.treeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(menuReq(const QPoint &)));
}

CListWin::~CListWin()
{

}

void CListWin::menuReq(const QPoint &p) {
	QModelIndex i = ui.treeView->indexAt(p);

	QPoint mouse_pos = QCursor::pos(); 
	QPoint pos = mapToGlobal(p);
	emit showMenu(pos, i);
}
