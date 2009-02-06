#ifndef _I_CORE_H
#define _I_CORE_H

/*!
\mainpage

 \section Introduction

 SAJE - is a pluginized platform (including a Jabber client implementation) by Scott Ellis (<a href="mailto:mail@scottellis.com.au">mail@scottellis.com.au</a>).

 \section Architecture

 The SAJE software relies on a plugin based architecture, where functionality is modularized and separated into independent plugins. The core program 
 is accessible via the CoreI interface definition, and provides a minimal set of common functions as well as the ability to obtain interfaces
 to other modules. Every module is passed a reference to the core within it's PluginI::load method which is called when the module is
 loaded by the system.

 The core uses a two-step load system to allow for circular dependencies. A second method, PluginI::modules_loaded, is called on every module after 
 all plugins have been loaded and their load methods have been called.

 In order for a module to provide functionality to other modules, it must subclass PluginI and implement the necessary pure virtual functions defined 
 therein.

 \section Licencing
 The SAGE software, in source and binary form, is copyright &copy; <a href="http://www.scottellis.com.au">Scott Ellis</a>. It is licenced under
 the <a href="http://www.gnu.org/licenses/gpl-3.0.html">GPL v3<a/>.
*/

#include <QObject>
#include <QStringList>
#include <QPointer>
#include <QApplication>

class PluginI;

/**
* Interface to core functions.
*/
class CoreI: public QObject {
	Q_OBJECT

public:
	CoreI(QApplication *parent = 0): QObject(parent) {}
	virtual ~CoreI() {}

	/// obtain an interface for a module - use the INAME_<module name> #define in the module's header file as the 'name' parameter
	virtual Q_INVOKABLE PluginI *get_interface(const QString &pluginId) = 0;

	/// get a list of all loaded interface names
	virtual Q_INVOKABLE QStringList get_interface_ids() = 0;

	/// get a list of all loaded interface class names
	virtual Q_INVOKABLE QStringList get_interface_class_names() = 0;

	/// retreive the config file directory name
	virtual Q_INVOKABLE const QString &get_config_dir() = 0;

	/// return the current time as seconds since 1/1/1970, to millisecond or better resolution, as a double
	virtual Q_INVOKABLE double current_time() = 0;

	/// make a relative path absolute - does nothing to an absolute path
	virtual Q_INVOKABLE QString make_absolute(const QString &path) = 0;

	/// encrypt a string
	virtual Q_INVOKABLE QString encrypt(const QString &source, const QString &key = "bad key") = 0;
	/// decrypt a string
	virtual Q_INVOKABLE QString decrypt(const QString &source, const QString &key = "bad key") = 0;

	// return version info
	virtual Q_INVOKABLE QString version() = 0;

	// set application style sheet and emit styleSheetSet
	virtual Q_INVOKABLE void setStyleSheet(const QString &styleSheet) {
		if(qApp) qApp->setStyleSheet(styleSheet);
		emit styleSheetSet(styleSheet);
	}
signals:
	void styleSheetSet(const QString &styleSheet);
};

Q_DECLARE_INTERFACE(CoreI, "au.com.sje.CoreI/1.0")

#endif
