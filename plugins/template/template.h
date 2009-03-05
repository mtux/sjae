#ifndef __TEMPLATE_H
#define __TEMPLATE_H

#include <plugin_i.h>		
#include <template_i.h>				// INT_CODE
#include <options_i.h> 				// OPT_CODE
#include "templateoptions.h" 		// OPT_CODE

class Template: public PluginI 		// NO_INT_CODE
class Template: public TemplateI 	// INT_CODE
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

protected slots:				// OPT_CODE
	void options_applied();		// OPT_CODE
	
protected:
	CoreI *core_i;
	TemplateOptions *opt;		// OPT_CODE
};

#endif // TEMPLATE

