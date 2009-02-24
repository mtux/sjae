#include "clistoptions.h"
#include <QDebug>

CListOptions::CListOptions(const Settings &settings, QWidget *parent)
	: OptionsPageI(parent), current_settings(settings)
{
	ui.setupUi(this);
	reset();
}

CListOptions::~CListOptions() {

}


bool CListOptions::apply() {
	current_settings.hide_offline = ui.chkHideOffline->isChecked();
	current_settings.hide_empty_groups = ui.chkHideEmptyGroups->isChecked();
	emit applied();
	return true;
}

void CListOptions::reset() {
	ui.chkHideOffline->setChecked(current_settings.hide_offline);
	ui.chkHideEmptyGroups->setChecked(current_settings.hide_empty_groups);
}

void CListOptions::on_chkHideOffline_stateChanged(int) {
	emit changed(true);
}

void CListOptions::on_chkHideEmptyGroups_stateChanged(int) {
	emit changed(true);
}