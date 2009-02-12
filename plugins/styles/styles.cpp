#include "styles.h"
#include <QtPlugin>
#include <QDebug>
#include <QSettings>
#include <QFile>

PluginInfo info = {
	0x600,
	"Styles",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Styles",
	0x00000001
};

/////////////////////////////////////////

void Styles::applyStyleSheet() {
	QString styleSheet = optionsWin->styleSheet();
	core_i->setStyleSheet(styleSheet);
	QSettings s;
	s.setValue("Styles/StyleSheet", styleSheet);
}

////////////////////////////////////////

Styles::Styles() {
}

Styles::~Styles() {
}

bool Styles::load(CoreI *core) {
	core_i = core;
	if((options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS)) == 0) return false;

	QSettings s;
	if(s.contains("Styles/StyleSheet"))
		qApp->setStyleSheet(s.value("Styles/StyleSheet").toString());
	else {
		QFile f(":/Resources/default.ss");
		if(f.open(QIODevice::ReadOnly))
			qApp->setStyleSheet(f.readAll());
	}

	optionsWin = new StylesOptions();
	options_i->add_page("Appearance/" + get_plugin_info().name, optionsWin);
	connect(optionsWin, SIGNAL(applied()), this, SLOT(applyStyleSheet()));
	return true;
}

bool Styles::modules_loaded() {
	return true;
}

bool Styles::pre_shutdown() {
	return true;
}

bool Styles::unload() {
	return true;
}

const PluginInfo &Styles::get_plugin_info() {
	return info;
}

/////////////////////////////



/////////////////////////////

Q_EXPORT_PLUGIN2(styles, Styles)

