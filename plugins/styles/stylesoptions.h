#ifndef STYLESOPTIONS_H
#define STYLESOPTIONS_H

#include <options_i.h>
#include "ui_stylesoptions.h"

class StylesOptions : public OptionsPageI
{
	Q_OBJECT

public:
	StylesOptions(QWidget *parent = 0);
	~StylesOptions();

	bool apply();
	void reset();

	QString styleSheet();
signals:
	void changed(bool valid = true);
	void applied();
protected slots:
	void loadStyleSheet(int i);

private:
	Ui::StylesOptionsClass ui;
	QString savedStyle;
};

#endif // STYLESOPTIONS_H
