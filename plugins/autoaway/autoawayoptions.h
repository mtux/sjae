#ifndef AUTOAWAYOPTIONS_H
#define AUTOAWAYOPTIONS_H

#include <options_i.h>
#include "ui_autoawayoptions.h"
#include <global_status.h>

class AutoAwayOptions : public OptionsPageI
{
	Q_OBJECT

public:
	class Settings {
	public:
		bool enable;
		int min;
		GlobalStatus status;
		bool restore;
	};

	AutoAwayOptions(const Settings &s, QWidget *parent = 0);
	~AutoAwayOptions();

	bool apply();
	void reset();

	Settings get_settings() {return current_settings;}

signals:
	void changed(bool valid = true);
	void applied();

private:
	Ui::AutoAwayOptionsClass ui;
	Settings current_settings;

};

#endif // AUTOAWAYOPTIONS_H
