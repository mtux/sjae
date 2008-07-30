#include "statusbar.h"
#include <QtPlugin>
#include <QMenu>

PluginInfo info = {
	0x600,
	"Status Bar",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Status Bar",
	0x00000001
};

StatusBar::StatusBar()
{

}

StatusBar::~StatusBar()
{

}

bool StatusBar::load(CoreI *core) {
	core_i = core;
	if((main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW)) == 0) return false;
	if((icons_i = (IconsI *)core_i->get_interface(INAME_ICONS)) == 0) return false;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;

	status_bar = new QStatusBar();
	main_win_i->set_status_bar(status_bar);
	return true;
}

bool StatusBar::modules_loaded() {
	//OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	//if(options_i)
		//options_i->add_page("User Interface/Contact List", new CListOptions(this));

	menuMapper = new QSignalMapper(this);
	connect(menuMapper, SIGNAL(mapped(QObject *)), this, SLOT(actionTriggered(QObject *)));
	
	connect(accounts_i, SIGNAL(account_added(const QString &, const QString &)), this, SLOT(account_added(const QString &, const QString &)));
	connect(accounts_i, SIGNAL(account_removed(const QString &, const QString &)), this, SLOT(account_removed(const QString &, const QString &)));

	QStringList proto_names = accounts_i->protocol_names();
	foreach(QString proto_name, proto_names) {
		QStringList account_ids = accounts_i->account_ids(proto_name);
		foreach(QString account_id, account_ids) {
			account_added(proto_name, account_id);
		}
	}
	return true;
}

void StatusBar::account_added(const QString &proto_name, const QString &account_id) {
	ProtocolI *proto = accounts_i->get_proto_interface(proto_name);
	QToolButton *tb = new QToolButton();
	tb->setIcon(icons_i->get_account_status_icon(proto, account_id, proto->get_status(account_id)));
	tb->setToolTip(proto->name() + ": " + account_id);
	QMenu *menu = new QMenu();
	connect(proto, SIGNAL(local_status_change(const QString &, const QString &, GlobalStatus)) , this, SLOT(local_status_change(const QString &, const QString &, GlobalStatus)));
	menu->addAction(icons_i->get_icon("Proto/" + proto->name()), proto->name() + ": " + account_id);
	menu->addSeparator();
	QList<GlobalStatus> statuses = proto->statuses();
	foreach(GlobalStatus gs, statuses) {
		QAction *a = menu->addAction(icons_i->get_account_status_icon(proto, account_id, gs), hr_status_name[gs]);
		a->setData(QVariantList() << proto->name() << account_id << gs);
		connect(a, SIGNAL(triggered()), menuMapper, SLOT(map()));
		menuMapper->setMapping(a, a);
	}
	tb->setMenu(menu);
	tb->setPopupMode(QToolButton::InstantPopup);
	status_bar->addWidget(tb);
	account_buttons[proto_name][account_id] = tb;
}

void StatusBar::account_removed(const QString &proto_name, const QString &account_id) {
	if(account_buttons.contains(proto_name) && account_buttons[proto_name].contains(account_id)) {
		QToolButton *tb = account_buttons[proto_name][account_id];
		status_bar->removeWidget(tb);
		menuMapper->removeMappings(tb->menu());
		delete tb;
		account_buttons[proto_name].remove(account_id);
		if(account_buttons[proto_name].size() == 0)
			account_buttons.remove(proto_name);
	}
}

void StatusBar::actionTriggered(QObject *action) {
	QAction *a = static_cast<QAction *>(action);
	QVariantList vl = a->data().toList();
	QString proto_name = vl.at(0).toString(),
		account_id = vl.at(1).toString();
	GlobalStatus gs = (GlobalStatus)vl.at(2).toInt();
	ProtocolI *proto = accounts_i->get_proto_interface(proto_name);
	proto->set_status(account_id, gs);
}

void StatusBar::local_status_change(const QString &proto_name, const QString &account_id, GlobalStatus gs) {
	if(account_buttons.contains(proto_name) && account_buttons[proto_name].contains(account_id)) {
		account_buttons[proto_name][account_id]->setIcon(icons_i->get_account_status_icon(accounts_i->get_proto_interface(proto_name), account_id, gs));
	}
}

bool StatusBar::pre_shutdown() {
	return true;
}

bool StatusBar::unload() {
	return true;
}

const PluginInfo &StatusBar::get_plugin_info() {
	return info;
}

/////////////////////////////



/////////////////////////////

Q_EXPORT_PLUGIN2(statusBar, StatusBar)
