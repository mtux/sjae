#ifndef OPTIONSTREE_H
#define OPTIONSTREE_H

#include <QTreeWidget>
#include "ui_optionstree.h"

class OptionsTree : public QTreeWidget
{
	Q_OBJECT

public:
	OptionsTree(QWidget *parent = 0);
	~OptionsTree();

private:
	Ui::OptionsTreeClass ui;
};

#endif // OPTIONSTREE_H
