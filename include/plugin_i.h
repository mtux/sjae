#ifndef _I_PLUGIN_H
#define _I_PLUGIN_H

#include "core_i.h"
#include <QStringList>

/**
* provides information about a plugin
*/
class PluginInfo {
public:
	unsigned int load_order;

	QString name; //!< human readable name
	QString author;
	QString author_email;
	QString website;
	QString description;
	quint32 version;
};

/**
* Implemented by all plugin modules.
Example:
\code
class SomePlugin: public QObject, public PluginInterface {
	Q_OBJECT
	Q_INTERFACES(PluginInterface)

public:
	SomePlugin();
	virtual ~SomePlugin();
	
	bool load(CoreI *core_i);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	
	const PluginInfo &get_plugin_info();	
protected:
	CoreI *core_i; 
	QPointer<ExampleI> ex_i;
};
\endcode

In the source code for the plugin, don't forget to #include <QtPlugin> and end the source file with a line of the form:
\code
Q_EXPORT_PLUGIN2(somePlugin, SomePlugin)
\endcode

The plugin's load method might look something like this:
\code
bool SomePlugin::load(CoreI *c) {
	core_i = c;
	if((ex_i = (ExampleI *)core_i->get_interface(INAME_EXAMPLE)) == 0) return false;
	return true;
}
\endcode

In the example above, the core_i pointer passed to the plugin in the 'load' method is saved, so that at any time the
global functionality that it provides can be accessed.

Also in the load method, the example interface pointer 'ex_i' is set using a line of the form:
\code
if((ex_i = (ExampleI *)core_i->get_interface(INAME_EXAMPLE)) == 0) return false;
\endcode
Returning false in this case ensures that the plugin is not loaded if the example interface is not available. All plugins should
set the load_order member of their PluginInfo structure higher than the highest value of any plugin on which they depend, to ensure
that the required interfaces have been loaded earlier.

Wrapping the pointer in a QPointer class instance means that you can safely test the pointer for null - this will be the case if the plugin 
it references is unloaded, for example. See the Qt documentation for the QPointer class. Note that this mechanism is not thread-safe.

Interfaces to modules loaded after this one can be obtained from the saved CoreI pointer in the modules_loaded method.

Note that you should set TEMPLATE = lib in the qmake project file for plugins.

\b Subclassing

In order to provide an interface accessible to other modules in the system, you must create an interface which subclasses PluginI, and then
subclass that interface when creating your plugin. Other modules may then access your 
plugin by including the new interface header file, and obtaining a pointer via the CoreI::get_interface method passing in the interface name defined in your interface header file. 
It is safer to wrap these pointers in QPointer objects, although this is not thread safe, because in the event of a call to an 
unloaded module method only a null-pointer exception will be thrown at run-time, instead of access violations or worse.

*/

#define INAME_GENERIC	"GenericInterface"
class PluginI: public QObject {
public:
	/// return the interface name implemented by this plugin - override in subclass interfaces
	virtual const QString get_interface_name() const {return INAME_GENERIC;}

	/// this is called when your plugin is loaded...save the core interface pointer for use during execution, and return false to prevent loading
	virtual bool load(CoreI *core) = 0;

	/// this is called when all modules have been loaded - so you can be sure plugin interfaces are available at this point if the plugin you're after is installed
	virtual bool modules_loaded() = 0;

	///  called when the core is about to unload all plugins - unregister with other plugins etc, since their interfaces may not be available in the unload method
	virtual bool pre_shutdown() = 0;

	/// called when you're unloaded - cleanup here
	virtual bool unload() = 0;

	/// return plugin information - load order  is used by the core (but if you use the modules_loaded and pre_shutdown handlers 'properly' it strictly shouldn't be necessary)
	virtual const PluginInfo &get_plugin_info() = 0;
};

Q_DECLARE_INTERFACE(PluginI, "au.com.sje.PluginI/1.0")

#endif
