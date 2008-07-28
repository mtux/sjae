#ifndef OPTIONSWIN_H
#define OPTIONSWIN_H

#include <QDialog>
#include "ui_optionswin.h"
#include <options_i.h>
#include <QStackedLayout>
#include <QTreeWidgetItem>

class OptionsWin : public QDialog
{
	Q_OBJECT

public:
	OptionsWin(QWidget *parent = 0);
	~OptionsWin();

	bool add_page(const QString &category, OptionsPageI *w);
protected slots:
	void apply();
	void cancel();

	void changed(bool valid);

	void treeSelectionChanged();

protected:
	QMap<QTreeWidgetItem *, OptionsPageI *> pages;
	QMap<OptionsPageI *, QString> categories;
	QStackedLayout *stack;
	QList<QWidget *> pages_invalid, pages_changed;

	void showEvent(QShowEvent *e);
	void set_button_states();
	
private:
	Ui::OptionsWinClass ui;
};

#endif // OPTIONSWIN_H
