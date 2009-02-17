#ifndef POPUPOPTIONS_H
#define POPUPOPTIONS_H

#include <options_i.h>
#include <popup_i.h>
#include "ui_popupoptions.h"

class PopupOptions : public OptionsPageI
{
	Q_OBJECT

public:
	class Settings {
	public:
		bool enabled;
		bool round_corners;
		QMap<QString, PopupI::PopupClass> classes;
	};

	PopupOptions(PopupI *popup_i, QWidget *parent = 0);
	~PopupOptions();

	bool apply();
	void reset();

	void set_settings(const Settings &s) {current_settings = s;}
	Settings get_settings() {return current_settings;}
signals:
	void changed(bool valid = true);
	void applied();

private:
	Ui::PopupOptionsClass ui;
	Settings current_settings;
	PopupI *popup_i;
private slots:
	void on_spnTimeout_valueChanged(int);
	void on_btnIcon_clicked();
	void on_btnPreview_clicked();
	void on_btnTextCol_clicked();
	void on_btnTitleCol_clicked();
	void on_btnBgCol_clicked();
	void on_lstClasses_currentRowChanged(int);
};

#endif // POPUPOPTIONS_H
