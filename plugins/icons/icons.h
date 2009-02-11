#ifndef ICONS_H
#define ICONS_H

#include <icons_i.h>
#include <QMap>

class icons: public IconsI
{
public:
	icons();
	~icons();

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();
	const PluginInfo &get_plugin_info();

	QPixmap get_icon(const QString &icon_id);
	bool add_icon(const QString &icon_id, const QPixmap &icon, const QString &label);
	bool add_alias(const QString &alias, const QString &icon_id, const QString &label);

	void setup_proto_icons(ProtocolI *proto);
	void setup_account_status_icons(Account *account);
	
	QPixmap get_account_status_icon(Account *account, GlobalStatus gs);

protected:
	CoreI *core_i;

	QMap<QString, QPixmap> icon_map;
	QMap<QString, QString> aliases;
	QMap<QString, QString> labels;
};

#endif // ICONS_H
