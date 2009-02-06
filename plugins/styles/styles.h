#ifndef __STYLES_H
#define __STYLES_H

#include <plugin_i.h>
#include <options_i.h>
#include "stylesoptions.h"

class Styles: public PluginI
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	Styles();
	~Styles();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

protected:
	CoreI *core_i;
	QPointer<OptionsI> options_i;

	StylesOptions *optionsWin;

protected slots:
	void applyStyleSheet();
};

#endif // STYLES

