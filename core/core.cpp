#include "core.h"
#include "arc4.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QDebug>
#include <QMultiMap>
#include <QThread>
#include <QTextStream>
#include <QDateTime>
#include <QApplication>
#include <QSettings>

#ifdef _WIN32  // for high performance timer
#include <windows.h>
#endif

#include <cstdio>

/* Convert LARGE_INTEGER to double */
#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))
#define TIME_GAIN		1


Core::Core(QApplication *parent): CoreI(parent) {

	if(parse_command_line())
		load_plugins();
	else
		qApp->quit();
	
#ifdef _WIN32
	LARGE_INTEGER hr_freq, hr_send_time;	
	if(QueryPerformanceFrequency(&hr_freq)) {
		QueryPerformanceCounter(&hr_send_time);
		double t = Li2Double(hr_send_time) / Li2Double(hr_freq);
		time_offset = QDateTime::currentDateTime().toTime_t() - t;
	} else
		time_offset = 0;
#else
	time_offset = 0;
#endif

	if(decrypt(encrypt("test", "some silly key"), "some silly key") != "test")
		qDebug() << "crypt broken";

	if(parent) connect(parent, SIGNAL(aboutToQuit()), this, SLOT(unload_plugins()));
}

Core::~Core() {
	//unload_plugins();
}

double Core::current_time() {
#ifdef _WIN32
	LARGE_INTEGER hr_freq, hr_send_time;	
	if(QueryPerformanceFrequency(&hr_freq)) {
		QueryPerformanceCounter(&hr_send_time);
		return Li2Double(hr_send_time) / Li2Double(hr_freq) * TIME_GAIN + time_offset;
	}
#endif
	QDateTime current = QDateTime::currentDateTime();
	uint dt = current.toTime_t();
	return dt + current.time().msec() / 1000.0;
}

QString Core::make_absolute(const QString &path) {
	if(path.startsWith("/") || (path.length() >= 2 && path.at(1) == ':'))
		return path;

	return qApp->applicationDirPath() + "/" + path;
}

QByteArray crypt(const QByteArray b, const QString &key) {
	QString k;
	if(key == "bad key" || key.isEmpty()) {
		k = "hm this ain't good, not good ta all";
		qWarning() << "core: crypt with bad key";
	} else
		k = key;

	QByteArray bk = k.toUtf8();

	char *out = (char *)malloc(b.length());
	arc4_ctx ctx;
	arc4_init(&ctx, bk.data(), bk.length());
	arc4_crypt(&ctx, (char *)b.data(), out, b.length());

	QByteArray ret = QByteArray(out, b.length());
	free(out);
	return ret;
}

Q_INVOKABLE QString Core::encrypt(const QString &source, const QString &key) {
	QByteArray b = source.toUtf8();
	if(b.length() < 20) {
		int len = b.length();
		b.resize(20);
		for(int i = len; i < 20; i++)
			b[i] = 0;
	}
	return QString(crypt(b, key).toBase64());
}

Q_INVOKABLE QString Core::decrypt(const QString &source, const QString &key) {
	return QString(crypt(QByteArray::fromBase64(source.toUtf8()), key));
}

PluginI *Core::get_interface(const QString &name) {
	if(interfaces.contains(name))
		return interfaces.value(name);
	return 0;
}

QStringList Core::get_interface_ids() {
	return interfaces.keys();
}

QStringList Core::get_interface_class_names() {
	QStringList ret;
	QMapIterator<QString, PluginI *> i(interfaces);
	while(i.hasNext()) {
		ret << i.value()->metaObject()->className();
		i.next();
	}
	return ret;
}

void Core::show_help() {
	QString prog_name = QCoreApplication::instance()->arguments().at(0);
	QTextStream out(stdout);
	out << endl;
	out << "Usage: " << prog_name << " [options]" << endl;
	out << "options:" << endl;
	
	out << "-h, --help\t\t" << "print this help message and exit" << endl;
	out << "-pd, --plugin_dir\t" << "specify the directory where plugins are located" << endl;
	out << "-cd, --config_dir\t" << "specify the directory where config files are located" << endl;
	out << endl;
}

bool Core::parse_command_line() {
	QStringList prog_args = qApp->arguments();

	int index;
	if((index = prog_args.indexOf("-h")) != -1 ||  (index = prog_args.indexOf("--help")) != -1) {
		show_help();
		return false; // don't run
	}

	QDir dir;
	bool ok = true;
	if((index = prog_args.indexOf("-pd")) != -1 ||  (index = prog_args.indexOf("--plugin_dir")) != -1) {
		if(prog_args.size() > index + 1) {
			if(!dir.cd(prog_args.at(index + 1))) {
				ok = false;
				qDebug() << "invalid plugins directory";
			}
		} else {
			ok = false;
			qDebug() << "no plugins directory specified";
		}
		
		if(ok) plugin_dir = dir.absolutePath();
	} else {
		dir.cd(QCoreApplication::applicationDirPath());
		if(!dir.cd("plugins")) {
			ok = false;
			qDebug() << "no plugins directory found";
		}
		plugin_dir = dir.absolutePath();
	}

	if((index = prog_args.indexOf("-cd")) != -1 || (index = prog_args.indexOf("--config_dir")) != -1) {
		if(prog_args.size() > index + 1) {
			if(!dir.cd(prog_args.at(index + 1))) {
				qDebug() << "invalid config directory";
			}
		} else {
			qDebug() << "no config directory specified";
		}
		
		if(ok) {
			config_dir = dir.absolutePath();
			QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config_dir);
		}
	} else {
		//dir.cd(QCoreApplication::applicationDirPath());
		QSettings s;
		dir = QFileInfo(s.fileName()).dir();
		if(!dir.exists()) {
		//if(!dir.cd("config")) {
			qDebug() << "no config directory found";
		}
		config_dir = dir.absolutePath();
	}
	
	qDebug() << "plugin_dir: " << plugin_dir;
	qDebug() << "config_dir: " << config_dir;
	return ok;
}

bool Core::load_plugins() {
	PluginI *iface;
	PluginInfoEx pix;
	QDir pluginDir(plugin_dir);	
	foreach(QString file_name, pluginDir.entryList(QDir::Files)) {
		QPluginLoader loader(pluginDir.absoluteFilePath(file_name));
                if((iface = qobject_cast<PluginI *>(loader.instance())) != 0) {
			const PluginInfo &pi = iface->get_plugin_info();
			qDebug() << "Got plugin info for " << pi.name;
			pix.load_order = pi.load_order;
			pix.name = pi.name;
			pix.iface = iface;
			pix.filename = file_name;
			load_order_map.insert(pi.load_order, pix);
                } else
                    qDebug() << "failed to load" << file_name << loader.errorString();
	}

	// load in order, but record the db interface so that plugins loaded after it can be disabled via db settings
	QMutableMapIterator<int, PluginInfoEx> i(load_order_map);
	bool unload;
	QString iface_name;
	int gen_index = 1;
	while(i.hasNext()) {
		i.next();
		pix = i.value();
		iface = pix.iface;
		unload = false;
		iface_name = iface->get_interface_name();
		// allow for multiple plugins to use the INAME_GENERIC interface name
		if(iface_name == INAME_GENERIC) {
			iface_name += QString("%1").arg(gen_index++);
			qDebug() << "Generating unique interface name (" << pix.name << "): " << iface_name;
		}
		if(interfaces.contains(iface_name)) {
			qDebug() << "Interface exists (" << pix.load_order << "): " << pix.name << "/" << iface_name;
			i.remove();
			unload = true;

		} else if(iface->load(this)) {
			qDebug() << "Loaded " << pix.name << " (" << pix.load_order << "): " << pix.name;
			// fill name-to-interface map
			interfaces[iface_name] = iface;
		} else {
			qDebug() << "Not loaded " << pix.name << " (" << pix.load_order << "): " << pix.name;
			i.remove();
			unload = true;
		}

		if(unload) {
			QPluginLoader loader(pix.filename);
			loader.unload();
		}

		qApp->processEvents();
	}

	// alert plugins that all modules have been loaded, and interfaces are available
	i.toFront();
	while(i.hasNext()) {
		i.next();
		iface = i.value().iface;
		iface->modules_loaded();
		qApp->processEvents();
	}

	return true;
}

bool Core::unload_plugins() {
	QMapIterator<int, PluginInfoEx> i(load_order_map);
	// shut down and unload in reverse order
	i.toBack();
	while(i.hasPrevious()) {
		i.previous();
		i.value().iface->pre_shutdown();
	}

	i.toBack();
	while(i.hasPrevious()) {
		i.previous();
		i.value().iface->unload();

		QPluginLoader loader(i.value().filename);
		loader.unload();
		qDebug() << "Unloaded: " << i.value().name;
	}
	
	return true;
}
