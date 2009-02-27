#ifndef CLISTOPTIONS_H
#define CLISTOPTIONS_H

#include <options_i.h>
#include <clist_i.h>
#include "ui_clistoptions.h"
#include <QPointer>

class CListOptions : public OptionsPageI
{
	Q_OBJECT

public:
	class Settings {
	public:
		bool hide_offline, hide_empty_groups;
	};

	CListOptions(const Settings &settings, QWidget *parent = 0);
	~CListOptions();

	bool apply();
	void reset();

	Settings currentSettings() {return current_settings;}
signals:
	void applied();

protected:
	QPointer<CListI> clist_i;
	Ui::CListOptionsClass ui;
	Settings current_settings;
private slots:
	void on_chkHideEmptyGroups_stateChanged(int);
	void on_chkHideOffline_stateChanged(int);
};

#endif // CLISTOPTIONS_H
