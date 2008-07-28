#include "clistwin.h"
#include <QDebug>

CListWin::CListWin(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.treeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(menuReq(const QPoint &)));
}

CListWin::~CListWin()
{

}

void CListWin::menuReq(const QPoint &p) {
	QTreeWidgetItem *i = ui.treeWidget->itemAt(p);
	if(!i) return;

	QPoint mouse_pos = QCursor::pos(); //mapToGlobal(p);
	emit aboutToShowMenu(i);
	contactMenu.exec(mouse_pos);
}
