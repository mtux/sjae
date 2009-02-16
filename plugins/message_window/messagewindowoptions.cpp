#include "messagewindowoptions.h"

MessageWindowOptions::MessageWindowOptions(const Settings &s, bool enable_hist, QWidget *parent)
	: OptionsPageI(parent), current_settings(s), enable_history(enable_hist)
{
	ui.setupUi(this);

	ui.chkHistory->setEnabled(enable_history);
}

MessageWindowOptions::~MessageWindowOptions()
{

}

bool MessageWindowOptions::apply() {
	Settings s;
	if(ui.radShowPopup->isChecked()) s.show_style = Settings::SS_POPUP;
	else if(ui.radShowMin->isChecked()) s.show_style = Settings::SS_MINIMIZED;
	else s.show_style = Settings::SS_NONE;

	if(ui.chkHistory->isChecked() && ui.radHistoryDays->isChecked()) s.load_history = Settings::LH_TIME;
	else if(ui.chkHistory->isChecked() && ui.radHistoryCount->isChecked()) s.load_history = Settings::LH_COUNT;
	else s.load_history = Settings::LH_NONE;

	s.history_days = ui.spnHistoryDays->value();
	s.history_count = ui.spnHistoryCount->value();

	s.send_chat_state = ui.chkSendChatState->isChecked();

	current_settings = s;

	emit applied();
	return true;
}

void MessageWindowOptions::reset() {
	Settings s = current_settings;

	switch(s.show_style) {
		case Settings::SS_POPUP: ui.radShowPopup->setChecked(true); break;
		case Settings::SS_MINIMIZED: ui.radShowMin->setChecked(true); break;
		case Settings::SS_NONE: ui.radShowNone->setChecked(true); break;
	}

	ui.chkHistory->setChecked(s.load_history != Settings::LH_NONE);
	switch(s.load_history) {
		case Settings::LH_TIME: 
			ui.radHistoryDays->setChecked(true); 
			break;
		case Settings::LH_COUNT: 
			ui.radHistoryCount->setChecked(true); 
			break;
	}
	ui.spnHistoryDays->setValue(s.history_days);
	ui.spnHistoryCount->setValue(s.history_count);
	on_chkHistory_toggled(s.load_history != Settings::LH_NONE);

	ui.chkSendChatState->setChecked(s.send_chat_state);
}


void MessageWindowOptions::on_chkHistory_toggled(bool f)
{
	ui.radHistoryDays->setEnabled(enable_history && f);
	ui.radHistoryCount->setEnabled(enable_history && f);
	ui.spnHistoryDays->setEnabled(enable_history && f && ui.radHistoryDays->isChecked());
	ui.spnHistoryCount->setEnabled(enable_history && f && ui.radHistoryCount->isChecked());
}