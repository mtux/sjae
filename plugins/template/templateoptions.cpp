#include "templateoptions.h"

TemplateOptions::TemplateOptions(QWidget *parent)
	: OptionsPageI(parent)
{
	ui.setupUi(this);

	reset();

}

TemplateOptions::~TemplateOptions()
{

}

bool TemplateOptions::apply() {
	emit applied();
	return true;
}

void TemplateOptions::reset() {
}
