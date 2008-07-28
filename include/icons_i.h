#ifndef _I_ICONS_H
#define _I_ICONS_H

#include "accounts_i.h"
#include <QString>
#include <QPixmap>

#define INAME_ICONS	"IconsInterface"

class IconsI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	const QString get_interface_name() const {return INAME_ICONS;}

	virtual QPixmap get_icon(const QString &icon_id) = 0;
	virtual bool add_icon(const QString &icon_id, const QPixmap &icon, const QString &hr_label) = 0;
	virtual bool add_alias(const QString &alias, const QString &icon_id, const QString &hr_label) = 0;

	virtual void setup_proto_icons(ProtocolI *proto) = 0;
	virtual void setup_account_status_icons(ProtocolI *proto, const QString &account_id) = 0;
	
	virtual QPixmap get_account_status_icon(ProtocolI *proto, const QString &account_id, GlobalStatus gs) = 0;
};

#endif
