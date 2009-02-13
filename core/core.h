#include <plugin_i.h>
#include <QMap>

class PluginInfoEx: public PluginInfo{
public:
	PluginI *iface;
	QString filename;
};

class Core: public CoreI {
	Q_OBJECT
	Q_INTERFACES(CoreI)

protected:
	// map plugin  names to interfaces
	QMap<QString, PluginI *> interfaces;
	// for each load order value keep a list of plugins, so we can load in order and unload in reverse order
	QMultiMap<int, PluginInfoEx> load_order_map;

	QString plugin_dir, config_dir;
	double time_offset;

protected slots:
	void show_help();
	bool parse_command_line();

	bool load_plugins();
	bool unload_plugins();

public:
	Core(QApplication *parent = 0);
	virtual ~Core();
	
	Q_INVOKABLE PluginI *get_interface(const QString &name);
	Q_INVOKABLE QStringList get_interface_ids();
	Q_INVOKABLE QStringList get_interface_class_names();
	
	Q_INVOKABLE const QString &get_config_dir() {return config_dir;}

	Q_INVOKABLE double current_time();

	Q_INVOKABLE QString make_absolute(const QString &path);
	
	Q_INVOKABLE QString encrypt(const QString &source, const QString &key = "");
	Q_INVOKABLE QString decrypt(const QString &source, const QString &key = "");

	Q_INVOKABLE QString platform() {return "Saje";}
	Q_INVOKABLE QString version() {return "Alpha 8";}
};


