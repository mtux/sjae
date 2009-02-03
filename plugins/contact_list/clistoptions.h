#ifndef CLISTOPTIONS_H
#define CLISTOPTIONS_H

#include <options_i.h>
#include <clist_i.h>
#include "ui_clistoptions.h"
#include <QPointer>

class CListOptions : public OptionsPageI
{
	Q_OBJECT

public:
	CListOptions(CListI *clist_i, QWidget *parent = 0);
	~CListOptions();

	bool apply();
	void reset();

protected:
	QPointer<CListI> clist_i;
	Ui::CListOptionsClass ui;

private slots:
	void on_chkHideOffline_stateChanged(int);
};

#endif // CLISTOPTIONS_H
