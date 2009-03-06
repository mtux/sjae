#ifndef MENUSOPTIONS_H
#define MENUSOPTIONS_H

#include <options_i.h>
#include "ui_menusoptions.h"

class MenusOptions : public OptionsPageI
{
	Q_OBJECT

public:
	MenusOptions(QWidget *parent = 0);
	~MenusOptions();

	bool apply();
	void reset();

signals:
	void changed(bool valid = true);
	void applied();

private:
	Ui::MenusOptionsClass ui;
};

#endif // STYLESOPTIONS_H
