#include "popup.h"
#include <QtPlugin>
#include <QRect>
#include <QDesktopWidget>
#include <QSettings>
#include <QDesktopWidget>
#include <QDebug>

PluginInfo info = {
	0x600,
	"Popup",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"PopupNotify",
	0x00000001
};

Popup::Popup(): opt(0), nextWinId(1)
{

}

Popup::~Popup()
{

}

bool Popup::load(CoreI *core) {
	core_i = core;

	QSettings settings;
	current_settings.enabled = settings.value("Popup/Enabled", true).toBool();
	current_settings.round_corners = settings.value("Popup/RoundCorners", false).toBool();

	OptionsI *options_i = (OptionsI*)core_i->get_interface(INAME_OPTIONS);
	if(options_i) {
		opt = new PopupOptions(this);
		options_i->add_page("Appearance/Popups", opt);
		connect(opt, SIGNAL(applied()), this, SLOT(options_applied()));
	}

	PopupI::PopupClass p;
	p.name = "Default";
	p.icon = QIcon(":/Resources/Notepad.png");
	p.title = Qt::black;
	p.text = Qt::black;
	p.background = Qt::gray;
	p.listener = this;
	p.timeout = 10;
	register_class(p);

	p.name = "Warning";
	p.icon = QIcon(":/Resources/Alert.png");
	p.title = Qt::black;
	p.text = Qt::black;
	p.background = Qt::red;
	p.listener = this;
	p.timeout = 0;
	register_class(p);

	return true;
}

bool Popup::modules_loaded() {
	return true;
}

bool Popup::pre_shutdown() {
	foreach(PopupWin *win, windows)
		delete win;
	return true;
}

bool Popup::unload() {
	return true;
}

const PluginInfo &Popup::get_plugin_info() {
	return info;
}

/////////////////////////////

bool Popup::register_class(const PopupClass &c)  {
	if(current_settings.classes.contains(c.name)) return false;
	current_settings.classes[c.name] = c;

	QSettings settings;
	current_settings.classes[c.name].icon = QIcon(settings.value("Popup/Class/" + c.name + "/Icon", c.icon.pixmap(256, 256)).value<QPixmap>());
	current_settings.classes[c.name].title = settings.value("Popup/Class/" + c.name + "/TitleCol", c.title).value<QColor>();
	current_settings.classes[c.name].text = settings.value("Popup/Class/" + c.name + "/TextCol", c.text).value<QColor>();
	current_settings.classes[c.name].background = settings.value("Popup/Class/" + c.name + "/BackgroundCol", c.background).value<QColor>();
	current_settings.classes[c.name].timeout = settings.value("Popup/Class/" + c.name + "/Timeout", c.timeout).toInt();

	if(opt) opt->set_settings(current_settings);
	return true;
}

PopupI::PopupClass Popup::get_class(const QString &name) {
	if(current_settings.classes.contains(name))
		return current_settings.classes[name];
	return PopupClass();
}

int Popup::show_popup(const QString &className, const QString &title, const QString &text) {
	if(current_settings.enabled && current_settings.classes.contains(className)) {
		return show_custom(current_settings.classes[className], title, text, current_settings.round_corners);
	}
	return -1;
}

void Popup::close_popup(int id) {
	foreach(PopupWin *win, windows) {
		if(win->getId() == id) {
			win->closeManual();
			break;
		}
	}
}

int Popup::show_custom(const PopupI::PopupClass &c, const QString &title, const QString &text, bool round_corners) {
	int id = nextWinId++;
	PopupWin *win = new PopupWin(c, id, round_corners);
	connect(win, SIGNAL(closed(int)), this, SLOT(win_closed(int)));
	//connect(win, SIGNAL(resized()), this, SLOT(layoutPopups()));

	win->setContent(title, text);

	windows.append(win);
	layoutPopups();
	win->show();

	return id;
}

void Popup::layoutPopups() {
	QRect r, screen = QApplication::desktop()->availableGeometry();
	int y = screen.height();
	//qDebug() << "y:" << y;
	for(int i = windows.size() - 1; i >= 0; --i) {
		//qDebug() << "window #" << i << "height:" << windows.at(i)->height();
		y -= (windows.at(i)->height() + 6);
		//qDebug() << "new y:" << y;
		windows.at(i)->move(screen.width() - (windows.at(i)->width() + 6), y);
	}
}

void Popup::win_closed(int id) {
	for(int i = 0; i < windows.size(); i++) {
		if(windows.at(i)->getId() == id) {
			windows.removeAt(i);
			break;
		}
	}
	layoutPopups();
}

void Popup::popup_closed(int id, PopupI::PopupDoneType done) {
}

void Popup::options_applied() {
	QSettings settings;
	current_settings = opt->get_settings();

	settings.setValue("Popup/Enabled", current_settings.enabled);
	settings.setValue("Popup/RoundCorners", current_settings.round_corners);

	foreach(PopupI::PopupClass c, current_settings.classes) {
		settings.setValue("Popup/Class/" + c.name + "/Icon", c.icon.pixmap(256, 256));
		settings.setValue("Popup/Class/" + c.name + "/TitleCol", c.title);
		settings.setValue("Popup/Class/" + c.name + "/TextCol", c.text);
		settings.setValue("Popup/Class/" + c.name + "/BackgroundCol", c.background);
		settings.setValue("Popup/Class/" + c.name + "/Timeout", c.timeout);
	}
}

/////////////////////////////

Q_EXPORT_PLUGIN2(popup, Popup)

