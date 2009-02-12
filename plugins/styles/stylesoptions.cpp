#include "stylesoptions.h"
#include <QFile>
#include <QDir>

StylesOptions::StylesOptions(QWidget *parent)
	: OptionsPageI(parent)
{
	ui.setupUi(this);

	reset();

	connect(ui.cmbFiles, SIGNAL(currentIndexChanged(int)), this, SLOT(loadStyleSheet(int)));
	connect(ui.edStyleSheet, SIGNAL(textChanged()), this, SIGNAL(changed()));
}

StylesOptions::~StylesOptions()
{

}

void StylesOptions::loadStyleSheet(int i) {
	if(i == 0) {
		ui.edStyleSheet->setText(savedStyle);
	} else if(i == 1) {
		QFile f(":/Resources/default.ss");
		if(f.open(QIODevice::ReadOnly)) {
			ui.edStyleSheet->setText(f.readAll());
		}
	} else {
		QFile f(qApp->applicationDirPath() + "/styles/" + ui.cmbFiles->currentText());
		if(f.open(QIODevice::ReadOnly)) {
			ui.edStyleSheet->setText(f.readAll());
		}
	}
}

QString StylesOptions::styleSheet() {
	return ui.edStyleSheet->document()->toPlainText();
}

bool StylesOptions::apply() {
	emit applied();
	savedStyle = qApp->styleSheet();
	return true;
}

void StylesOptions::reset() {
	ui.cmbFiles->clear();

	QDir d(qApp->applicationDirPath() + "/styles");
	ui.cmbFiles->addItem("Current Style");
	ui.cmbFiles->addItem("Default");
	ui.cmbFiles->addItems(d.entryList(QDir::Files | QDir::Readable));
	ui.cmbFiles->setCurrentIndex(0);

	savedStyle = qApp->styleSheet();
	ui.edStyleSheet->setText(savedStyle);
}
