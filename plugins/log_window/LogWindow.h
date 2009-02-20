#ifndef _LOG_WINDOW
#define _LOG_WINDOW

#include <main_window_i.h>
#include "ui_log_window.h"

class LogFrame : public QWidget
{
	Q_OBJECT

public:
	LogFrame(QWidget *parent = 0, Qt::WFlags flags = 0);
	~LogFrame();

	void emit_message_add(const QString &message);
signals:
	void message_add(const QString &message);

public slots:
	void add_message(const QString &message);
	void showHideToggle();
	
private:
	Ui::logWindow ui;
};

class LogWindow: public PluginI
{
	Q_OBJECT
	Q_INTERFACES(PluginI)
public:
	LogWindow(void);
	virtual ~LogWindow(void);

	bool load(CoreI *core);
	bool modules_loaded();
	bool pre_shutdown();
	bool unload();

	const PluginInfo &get_plugin_info();

protected:
	CoreI *core_i;
	QPointer<MainWindowI> main_win_i;
	LogFrame *log_frame;
};

#endif