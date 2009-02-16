#ifndef MESSAGEWINDOWOPTIONS_H
#define MESSAGEWINDOWOPTIONS_H

#include <options_i.h>
#include "ui_messagewindowoptions.h"

class MessageWindowOptions : public OptionsPageI
{
	Q_OBJECT

public:
	class Settings {
	public:
		typedef enum {SS_POPUP, SS_MINIMIZED, SS_NONE} ShowStyleType;
		typedef enum {LH_TIME, LH_COUNT, LH_NONE} LoadHistoryType;

		ShowStyleType show_style;
		LoadHistoryType load_history;
		int history_days, history_count;

		bool send_chat_state;
	};

	MessageWindowOptions(const Settings &s, bool enable_history, QWidget *parent = 0);
	~MessageWindowOptions();

	bool apply();
	void reset();

	Settings get_settings() {return current_settings;}

signals:
	void changed(bool valid = true);
	void applied();

private:
	Ui::MessageWindowOptionsClass ui;
	Settings current_settings;
	bool enable_history;

private slots:
	void on_chkHistory_toggled(bool);
};

#endif // MESSAGEWINDOWOPTIONS_H
