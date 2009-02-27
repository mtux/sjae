#ifndef AUTORECONNECTOPTIONS_H
#define AUTORECONNECTOPTIONS_H

#include <options_i.h>
#include "ui_autoreconnectoptions.h"

class AutoReconnectOptions : public OptionsPageI
{
	Q_OBJECT

public:
	class Settings {
	public:
		bool enable;
	};

	AutoReconnectOptions(const Settings &s, QWidget *parent = 0);
	~AutoReconnectOptions();
	bool apply();
	void reset();
	Settings get_settings() {return current_settings;}
signals:
	void changed(bool valid = true);
	void applied();
private:
	Ui::AutoReconnectOptionsClass ui;
	Settings current_settings;
};

#endif // AUTORECONNECTOPTIONS_H
