#ifndef __TEMPLATE_H
#define __TEMPLATE_H

#include <plugin_i.h>

class Template: public PluginI
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	Template();
	~Template();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

protected:
	CoreI *core_i;
};

#endif // TEMPLATE

