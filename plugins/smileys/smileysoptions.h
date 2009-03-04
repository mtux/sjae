#ifndef SMILEYSOPTIONS_H
#define SMILEYSOPTIONS_H

#include <options_i.h>
#include "ui_smileysoptions.h"

class SmileysOptions : public OptionsPageI
{
	Q_OBJECT
public:
	class Settings {
	public:
		bool enable;
		QMap<QString, QString> subs;
	};

	SmileysOptions(const Settings &s, QWidget *parent = 0);
	~SmileysOptions();

	bool apply();
	void reset();

	Settings get_settings() {return current_settings;}
signals:
	void changed(bool valid = true);
	void applied();
private:
	Ui::SmileysOptions *m_ui;
	Settings current_settings;
	QMap<QString, QString> subs;
private slots:
void on_listWidget_itemSelectionChanged();
	void on_btnDel_clicked();
		void on_btnAdd_clicked();
};

#endif // SMILEYSOPTIONS_H
