#ifndef MAINWINOPTIONS_H
#define MAINWINOPTIONS_H

#include <options_i.h>
#include "ui_mainwinoptions.h"

class MainWinOptions : public OptionsPageI
{
	Q_OBJECT

public:
	struct Settings {
		bool hide_toolbar;
		bool hide_frame;
		bool tool_window;
		int trans_percent;
		bool round_corners;
	};

	MainWinOptions(const Settings &settings, QWidget *parent = 0);
	~MainWinOptions();

	bool apply();
	void reset();

	Settings get_settings();
signals:
	void changed(bool valid = true);
	void applied();

private:
	Ui::MainWinOptionsClass ui;
	Settings current_settings;
};

#endif // MAINWINOPTIONS_H
