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
	menuMapper = new QSignalMapper(this);
	connect(menuMapper, SIGNAL(mapped(QObject *)), this, SLOT(actionTriggered(QObject *)));

	core_i = core;
	if((main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW)) == 0) return false;
	if((icons_i = (IconsI *)core_i->get_interface(INAME_ICONS)) == 0) return false;
	if((accounts_i = (AccountsI *)core_i->get_interface(INAME_ACCOUNTS)) == 0) return false;

	if((events_i = (EventsI *)core_i->get_interface(INAME_EVENTS)) == 0) return false;
	events_i->add_event_listener(this, UUID_ACCOUNT_CHANGED);

	status_bar = new QStatusBar();
	main_win_i->set_status_bar(status_bar);
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
		account_id = ac.account->account_id;
	if(ac.removed) {
		if(account_buttons.contains(proto_name) && account_buttons[proto_name].contains(account_id)) {
			QToolButton *tb = account_buttons[proto_name][account_id];
			status_bar->removeWidget(tb);
			menuMapper->removeMappings(tb->menu());
			delete tb;
			account_buttons[proto_name].remove(account_id);
			if(account_buttons[proto_name].size() == 0)
				account_buttons.remove(proto_name);
		}
	} else {
		bool new_account = account_buttons.contains(proto_name) == false || account_buttons[proto_name].contains(account_id) == false;
		if(new_account) {
			QToolButton *tb = new QToolButton();
			tb->setIcon(icons_i->get_account_status_icon(ac.account, ac.account->status));
			tb->setToolTip(proto_name + ": " + account_id);
			QMenu *menu = new QMenu();
			//connect(proto, SIGNAL(local_status_change(const QString &, const QString &, GlobalStatus)) , this, SLOT(local_status_change(const QString &, const QString &, GlobalStatus)));
			menu->addAction(icons_i->get_icon("Proto/" + proto_name), proto_name + ": " + account_id);
			menu->addSeparator();
			QList<GlobalStatus> statuses = ac.account->proto->statuses();
			foreach(GlobalStatus gs, statuses) {
				QAction *a = menu->addAction(icons_i->get_account_status_icon(ac.account, gs), hr_status_name[gs]);
				a->setData(QVariantList() << proto_name << account_id << gs);
				connect(a, SIGNAL(triggered()), menuMapper, SLOT(map()));
				menuMapper->setMapping(a, a);
			}
			tb->setMenu(menu);
			tb->setPopupMode(QToolButton::InstantPopup);
			status_bar->addWidget(tb);
			account_buttons[proto_name][account_id] = tb;
		} else {
			account_buttons[proto_name][account_id]->setIcon(icons_i->get_account_status_icon(ac.account, ac.account->status));
		}
	}
	return true;
}

void StatusBar::actionTriggered(QObject *action) {
	QAction *a = static_cast<QAction *>(action);
	QVariantList vl = a->data().toList();
	QString proto_name = vl.at(0).toString(),
		account_id = vl.at(1).toString();
	GlobalStatus gs = (GlobalStatus)vl.at(2).toInt();
	Account *acc = accounts_i->account_info(proto_name, account_id);

	AccountStatusReq as(acc, gs, this);
	events_i->fire_event(as);
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
