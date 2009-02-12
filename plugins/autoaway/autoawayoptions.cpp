#include "autoawayoptions.h"

AutoAwayOptions::AutoAwayOptions(const Settings &s, QWidget *parent)
	: OptionsPageI(parent), current_settings(s)
{
	ui.setupUi(this);

	each_contact_status(status)
		ui.cmbStatus->addItem(hr_status_name[status]);

	connect(ui.chkEnable, SIGNAL(clicked()), this, SIGNAL(changed()));
	connect(ui.chkRestore, SIGNAL(clicked()), this, SIGNAL(changed()));
	connect(ui.cmbStatus, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
	connect(ui.spnMin, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
}

AutoAwayOptions::~AutoAwayOptions()
{

}

bool AutoAwayOptions::apply() {
	current_settings.enable = ui.chkEnable->isChecked();
	current_settings.min = ui.spnMin->value();
	current_settings.restore = ui.chkRestore->isChecked();
	current_settings.status = (GlobalStatus)ui.cmbStatus->currentIndex();
	emit applied();
	return true;
}

void AutoAwayOptions::reset() {
	ui.chkEnable->setChecked(current_settings.enable);
	ui.chkRestore->setChecked(current_settings.restore);
	ui.cmbStatus->setCurrentIndex((int)current_settings.status);
	ui.spnMin->setValue(current_settings.min);
}
