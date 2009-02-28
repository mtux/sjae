#include "autoreconnectoptions.h"

AutoReconnectOptions::AutoReconnectOptions(const Settings &s, QWidget *parent)
	: OptionsPageI(parent), current_settings(s)
{
	ui.setupUi(this);

	connect(ui.chkEnable, SIGNAL(clicked()), this, SIGNAL(changed()));
}

AutoReconnectOptions::~AutoReconnectOptions()
{

}

bool AutoReconnectOptions::apply() {
	current_settings.enable = ui.chkEnable->isChecked();
	emit applied();
	return true;
}

void AutoReconnectOptions::reset() {
	ui.chkEnable->setChecked(current_settings.enable);
}
