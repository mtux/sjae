#include "menusoptions.h"

MenusOptions::MenusOptions(QWidget *parent)
	: OptionsPageI(parent)
{
	ui.setupUi(this);

	reset();

}

MenusOptions::~MenusOptions()
{

}

bool MenusOptions::apply() {
	emit applied();
	return true;
}

void MenusOptions::reset() {
}
