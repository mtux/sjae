#include "icons.h"
#include <global_status.h>
#include <QtPlugin>

PluginInfo info = {
	0x100,
	"Icons",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Icons",
	0x00000001
};


icons::icons()
{

}

icons::~icons()
{

}

const char *def_icon[] = {
	"dot_grey",
	"dot_green",
	"dot_lgreen",
	"dot_yellow",
	"dot_orange",
	"dot_brown",
	"dot_red",
	"dot_orange",
	"dot_orange",
	"generic"
};

bool icons::load(CoreI *core) {
	core_i = core;
	add_icon("generic", QPixmap(":/icons/Resources/s.png"), "General/default");

	add_icon("dot_blue", QPixmap(":/icons/Resources/c_blue.png"), "General/blue dot");
	add_icon("dot_brown", QPixmap(":/icons/Resources/c_brown.png"), "General/brown dot");
	add_icon("dot_green", QPixmap(":/icons/Resources/c_green.png"), "General/green dot");
	add_icon("dot_lgreen", QPixmap(":/icons/Resources/c_lgreen.png"), "General/light green dot");
	add_icon("dot_grey", QPixmap(":/icons/Resources/c_grey.png"), "General/grey dot");
	add_icon("dot_cyan", QPixmap(":/icons/Resources/c_cyan.png"), "General/cyan dot");
	add_icon("dot_magenta", QPixmap(":/icons/Resources/c_magenta.png"), "General/magenta dot");
	add_icon("dot_red", QPixmap(":/icons/Resources/c_red.png"), "General/red dot");
	add_icon("dot_yellow", QPixmap(":/icons/Resources/c_yellow.png"), "General/yellow dot");
	add_icon("dot_orange", QPixmap(":/icons/Resources/c_orange.png"), "General/orange dot");

	each_status(gs) {
		add_alias(status_name[gs], def_icon[gs], QString("Status/") + hr_status_name[gs]);
	}
	return true;
}

bool icons::modules_loaded() {
	return true;
}

bool icons::pre_shutdown() {
	return true;
}

bool icons::unload() {
	return true;
}

const PluginInfo &icons::get_plugin_info() {
	return info;
}

QPixmap icons::get_icon(const QString &icon_id) {
	if(aliases.contains(icon_id))
		return icon_map[aliases[icon_id]];
	if(icon_map.contains(icon_id))
		return icon_map[icon_id];
	return icon_map["generic"];
}

bool icons::add_icon(const QString &icon_id, const QPixmap &icon, const QString &label) {
	if(!icon_map.contains(icon_id)) {
		icon_map[icon_id] = icon;
		labels[icon_id] = label;
		return true;
	}
	return false;
}

bool icons::add_alias(const QString &alias, const QString &icon_id, const QString &label) {
	if(aliases.contains(alias)) return false;
	if(icon_map.contains(alias)) return false;

	QString a = icon_id;
	while(aliases.contains(a)) a = aliases[a];
	if(!icon_map.contains(a)) return false;

	aliases[alias] = a;
	labels[alias] = label;
	return true;
}

void icons::setup_proto_icons(ProtocolI *proto) {
	QList<GlobalStatus> statuses = proto->statuses();
	foreach(GlobalStatus status, statuses) {
		add_alias("Proto/" + proto->name() + "/" + status_name[status], status_name[status], "Protocols/" + proto->name() + "/" + hr_status_name[status]);
	}
	add_alias("Proto/" + proto->name(), "generic", "Protocols/" + proto->name());
}

void icons::setup_account_status_icons(Account *account) {
	QList<GlobalStatus> statuses = account->proto->statuses();
	foreach(GlobalStatus status, statuses) {
		add_alias("Proto/" + account->proto->name() + "/Account/" + account->account_id + "/" + status_name[status], 
			"Proto/" +account->proto->name() + "/" + status_name[status], 
			"Protocols/" + account->proto->name() + "/Accounts/" + account->account_id + "/" + hr_status_name[status]);
	}
}

QPixmap icons::get_account_status_icon(Account *account, GlobalStatus gs) {
	//if(!proto) return get_icon(status_name[gs]);
	return get_icon("Proto/" + account->proto->name() + "/Account/" + account->account_id + "/" + status_name[account->proto->closest_status_to(gs)]);
}

Q_EXPORT_PLUGIN2(iconsPlugin, icons)
