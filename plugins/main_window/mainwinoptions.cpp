#include "mainwinoptions.h"

MainWinOptions::MainWinOptions(const Settings &settings, QWidget *parent)
	: OptionsPageI(parent), current_settings(settings)
{
	ui.setupUi(this);

	connect(ui.chkRoundCorners, SIGNAL(clicked()), this, SIGNAL(changed()));
	connect(ui.chkNoFrame, SIGNAL(clicked()), this, SIGNAL(changed()));
	connect(ui.chkNoToolbar, SIGNAL(clicked()), this, SIGNAL(changed()));
	connect(ui.chkToolWin, SIGNAL(clicked()), this, SIGNAL(changed()));
	connect(ui.sldTrans, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
	connect(ui.chkOnTop, SIGNAL(clicked()), this, SIGNAL(changed()));
	reset();
}

MainWinOptions::~MainWinOptions()
{

}

bool MainWinOptions::apply() {
	current_settings.hide_toolbar = ui.chkNoToolbar->isChecked();
	current_settings.hide_frame = ui.chkNoFrame->isChecked();
	current_settings.tool_window = ui.chkToolWin->isChecked();
	current_settings.trans_percent = ui.sldTrans->value();
	current_settings.round_corners = ui.chkRoundCorners->isChecked();
	current_settings.on_top = ui.chkOnTop->isChecked();
	emit applied();
	return true;
}

void MainWinOptions::reset() {
	ui.chkNoToolbar->setChecked(current_settings.hide_toolbar);
	ui.chkNoFrame->setChecked(current_settings.hide_frame);
	ui.chkToolWin->setChecked(current_settings.tool_window);
	ui.sldTrans->setValue(current_settings.trans_percent);
	ui.chkRoundCorners->setChecked(current_settings.round_corners);
	ui.chkOnTop->setChecked(current_settings.on_top);
}

MainWinOptions::Settings MainWinOptions::get_settings() {
	return current_settings;	
}

