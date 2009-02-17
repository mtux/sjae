#ifndef _I_POPUP_H
#define _I_POPUP_H

#include "plugin_i.h"

#include <QColor>
#include <QIcon>

#define INAME_POPUP		"PopupInterface"

class PopupI: public PluginI {
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	typedef enum {PDT_RIGHT_CLICK, PDT_LEFT_CLICK, PDT_TIMEOUT, PDT_MANUAL} PopupDoneType;

	const QString get_interface_name() const {return INAME_POPUP;}

	class PopupListener {
	public:
		virtual void popup_closed(int id, PopupDoneType done) = 0;
	};

	class PopupClass {
	public:
		QString name;
		QIcon icon;
		QColor title, text, background;
		PopupListener *listener;
		int timeout; // 0 means never, anything else is timeout in seconds
	};

	virtual bool register_class(const PopupClass &c) = 0;
	virtual PopupClass get_class(const QString &name) = 0;

	/// returns an int identifying the new popup, which will be passed to the class' PopupListener on close
	virtual int show_popup(const QString &className, const QString &title, const QString &text) = 0;
	virtual void close_popup(int id) = 0;

	virtual void show_preview(const PopupI::PopupClass &c, bool round_corners, const QString &title, const QString &text) = 0;
};

#endif
