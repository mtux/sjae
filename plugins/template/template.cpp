#include "template.h"
#include <QtPlugin>

PluginInfo info = {
	0x600,
	"Template",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Template",
	0x00000001
};

Template::Template()
{

}

Template::~Template()
{

}

bool Template::load(CoreI *core) {
	core_i = core;
	return true;
}

bool Template::modules_loaded() {
	OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);		// OPT_CODE
	if(options_i) {																// OPT_CODE
		opt = new TemplateOptions();											// OPT_CODE
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));			// OPT_CODE
		options_i->add_page("Template", opt);									// OPT_CODE
	}																			// OPT_CODE
	return true;
}

bool Template::pre_shutdown() {
	return true;
}

bool Template::unload() {
	return true;
}

const PluginInfo &Template::get_plugin_info() {
	return info;
}

/////////////////////////////

void Template::options_applied() {												// OPT_CODE
}																				// OPT_CODE

/////////////////////////////

Q_EXPORT_PLUGIN2(template, Template)

