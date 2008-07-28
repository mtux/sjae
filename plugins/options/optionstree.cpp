#include "optionstree.h"

OptionsTree::OptionsTree(QWidget *parent)
	: QTreeWidget(parent)
{
	ui.setupUi(this);
	setColumnCount(1);
	QTreeWidgetItem *header = new QTreeWidgetItem();
	setHeaderItem(header);
	header->setHidden(true);

}

OptionsTree::~OptionsTree()
{

}
