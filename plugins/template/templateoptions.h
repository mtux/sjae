#ifndef TEMPLATEOPTIONS_H
#define TEMPLATEOPTIONS_H

#include <options_i.h>
#include "ui_templateoptions.h"

class TemplateOptions : public OptionsPageI
{
	Q_OBJECT

public:
	TemplateOptions(QWidget *parent = 0);
	~TemplateOptions();

	bool apply();
	void reset();

signals:
	void changed(bool valid = true);
	void applied();

private:
	Ui::TemplateOptionsClass ui;
};

#endif // STYLESOPTIONS_H
