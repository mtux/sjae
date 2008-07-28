#include "clistoptions.h"
#include <QDebug>

CListOptions::CListOptions(CListI *clist, QWidget *parent)
	: OptionsPageI(parent), clist_i(clist)
{
	ui.setupUi(this);
	reset();
}

CListOptions::~CListOptions() {

}


bool CListOptions::apply() {
	clist_i->set_hide_offline(ui.chkHideOffline->isChecked());
	return true;
}

void CListOptions::reset() {
	ui.chkHideOffline->setChecked(clist_i->get_hide_offline());
}

void CListOptions::on_chkHideOffline_stateChanged(int) {
	emit changed(true);
}