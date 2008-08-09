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



/////////////////////////////

Q_EXPORT_PLUGIN2(template, Template)

