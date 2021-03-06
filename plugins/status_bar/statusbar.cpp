#include "statusbar.h"
#include <QtPlugin>
#include <QMenu>
#include <QSizePolicy>

PluginInfo info = {
	0x600,
	"Status Bar",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Status Bar",
	0x00000001
};

StatusBar::StatusBar():
	button_count(0)
{

}

StatusBar::~StatusBar()
{

}

bool StatusBar::load(CoreI *core) {
	menuMapper = new QSignalMapper(this);
	connect(menuMapper, SIGNAL(mapped(QObject *)), this, SLOT(actionTriggered(QObject *)));

	core_i = core;
	if((main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW)) == 0) return false;
	if((icons_i = (IconsI *)core_i->get_interface(INAME_ICONS)) == 0) return false;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;
	if((menus_i = (MenusI *)core_i->get_interface(INAME_MENUS)) == 0) return false;

	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);

	status_bar = new QStatusBar();
	main_win_i->set_status_bar(status_bar);

	toolbuttons = new QWidget(status_bar);
	toolbuttons->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	toolButtonLayout = new QHBoxLayout(toolbuttons);
	toolButtonLayout->setSpacing(6);
	toolButtonLayout->setMargin(4);
	toolbuttons->setLayout(toolButtonLayout);
	status_bar->addWidget(toolbuttons);

	QToolButton *tb = new QToolButton();
	tb->setIcon(icons_i->get_icon("generic"));
	tb->setToolTip("Global Status");

	QMenu *menu = menus_i->get_menu("Global Status");
	//connect(proto, SIGNAL(local_status_change(const QString &, const QString &, GlobalStatus)) , this, SLOT(local_status_change(const QString &, const QString &, GlobalStatus)));
	each_contact_status(gs) {
		QAction *a = menus_i->add_menu_action("Global Status", hr_status_name[gs], status_name[gs]);
		a->setData(gs);
		connect(a, SIGNAL(triggered()), menuMapper, SLOT(map()));
		menuMapper->setMapping(a, a);
	}
	menus_i->add_menu_action("Global Status", "Global Status", "generic");
	menus_i->add_menu_separator("Global Status", menu->actions().at(1));

	tb->setMenu(menu);
	tb->setPopupMode(QToolButton::InstantPopup);
	tb->hide();
	status_bar->addPermanentWidget(tb);
	globalButton = tb;

	return true;
}

bool StatusBar::modules_loaded() {
	//OptionsI *options_i = (OptionsI *)core_i->get_interface(INAME_OPTIONS);
	//if(options_i)
		//options_i->add_page("User Interface/Contact List", new CListOptions(this));
	
	return true;
}


bool StatusBar::event_fired(EventsI::Event &e) {
	AccountChanged &ac = static_cast<AccountChanged &>(e);
	QString proto_name = ac.account->proto->name(),
		account_id = ac.account->account_id,
		account_name = ac.account->account_name;
	if(ac.removed) {
		if(account_buttons.contains(proto_name) && account_buttons[proto_name].contains(account_id)) {
			QToolButton *tb = account_buttons[proto_name][account_id];
			status_bar->removeWidget(tb);
			menuMapper->removeMappings(tb->menu());
			delete tb;
			account_buttons[proto_name].remove(account_id);
			if(account_buttons[proto_name].size() == 0)
				account_buttons.remove(proto_name);
			button_count--;
			if(button_count <= 1)
				globalButton->hide();
		}
	} else {
		bool new_account = account_buttons.contains(proto_name) == false || account_buttons[proto_name].contains(account_id) == false;
		if(new_account) {
			QToolButton *tb = new QToolButton();
			tb->setIcon(icons_i->get_account_status_icon(ac.account, ac.account->status));
			tb->setToolTip(proto_name + ": " + account_name);

			QMenu *menu = menus_i->get_menu(proto_name + ": " + account_name);
			//connect(proto, SIGNAL(local_status_change(const QString &, const QString &, GlobalStatus)) , this, SLOT(local_status_change(const QString &, const QString &, GlobalStatus)));
			QList<GlobalStatus> statuses = ac.account->proto->statuses();
			foreach(GlobalStatus gs, statuses) {
				QAction *a = menus_i->add_menu_action(proto_name + ": " + account_name, hr_status_name[gs], "Proto/" + proto_name + "/Account/" + account_id + "/" + status_name[gs]);
				a->setData(QVariantList() << proto_name << account_id << gs);
				connect(a, SIGNAL(triggered()), menuMapper, SLOT(map()));
				menuMapper->setMapping(a, a);
			}
			menus_i->add_menu_action(proto_name + ": " + account_name, proto_name + ": " + account_name, "Proto/" + proto_name + "/Account/" + account_id);
			menus_i->add_menu_separator(proto_name + ": " + account_name, menu->actions().at(1));

			tb->setMenu(menu);
			tb->setPopupMode(QToolButton::InstantPopup);
			//status_bar->addWidget(tb);
			toolButtonLayout->addWidget(tb);
			account_buttons[proto_name][account_id] = tb;
			button_count++;
			if(button_count > 1)
				globalButton->show();
		} else {
			account_buttons[proto_name][account_id]->setIcon(icons_i->get_account_status_icon(ac.account, ac.account->status));
		}
	}
	return true;
}

void StatusBar::actionTriggered(QObject *action) {
	QAction *a = static_cast<QAction *>(action);
	QVariantList vl = a->data().toList();
	if(vl.size() == 3) {
		QString proto_name = vl.at(0).toString(),
			account_id = vl.at(1).toString();
		GlobalStatus gs = (GlobalStatus)vl.at(2).toInt();
		Account *acc = accounts_i->account_info(proto_name, account_id);

		AccountStatusReq as(acc, gs, this);
		events_i->fire_event(as);
	} else {
		GlobalStatus gs = (GlobalStatus)a->data().toInt();

		QStringList protos = accounts_i->protocol_names();
		foreach(QString proto_name, protos) {
			QStringList account_ids = accounts_i->account_ids(proto_name);
			foreach(QString account_id, account_ids) {
				Account *acc = accounts_i->account_info(proto_name, account_id);
				AccountStatusReq asr(acc, gs, this);
				events_i->fire_event(asr);
			}
		}
	}
}

bool StatusBar::pre_shutdown() {
	events_i->remove_event_listener(this, UUID_ACCOUNT_CHANGED);
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
