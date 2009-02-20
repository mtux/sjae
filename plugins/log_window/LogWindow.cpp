#include "LogWindow.h"
#include <QtPlugin>
#include <QTime>
#include <QTextStream>
#include <popup_i.h>

#include <iostream>

LogFrame *g_log_frame = 0;
QtMsgHandler oldHandler = 0;
QPointer<PopupI> popup_i;

void myMessageOutput(QtMsgType type, const char *msg) {
	QString timestr = QTime::currentTime().toString("HH:mm:ss.zzz");
	switch(type) {
		case QtDebugMsg:
			std::cerr << timestr.toUtf8().data() << " Debug: " << msg << std::endl;
			break;
		case QtWarningMsg:
			std::cerr << timestr.toUtf8().data() << " Warning: " << msg << std::endl;
			break;
		case QtCriticalMsg:
			std::cerr << timestr.toUtf8().data() << " Critical: " << msg << std::endl;
			break;
		case QtFatalMsg:
			std::cerr << timestr.toUtf8().data() << " Critical: " << msg << std::endl;
			abort();
	}
	if(g_log_frame) {
		QString str;
		QTextStream st_out(&str);
		switch (type) {
			case QtDebugMsg:
				st_out << "<font>" << timestr << " <b>Debug</b>: " << Qt::escape(msg) << "</font>";
				break;
			case QtWarningMsg:
				st_out << "<font color=#ff00ff>" << timestr << " <b>Warning</b>:" << Qt::escape(msg) << "</font>";
				if(popup_i) popup_i->show_popup("Warning", "Warning", msg);
				break;
			case QtCriticalMsg:
				st_out << "<font color=#ff0000>" << timestr << " <b>Critical</b>: " << Qt::escape(msg) << "</font>";
				if(popup_i) popup_i->show_popup("Warning", "Error", msg);
				break;
			// Handled above ...
			// case QtFatalMsg:
			default:
				break;
		}
		g_log_frame->emit_message_add(str);
	}

	if(oldHandler) oldHandler(type, msg);
}

LogFrame::LogFrame(QWidget *parent, Qt::WFlags flags)
	: QWidget(parent, flags)
{
	ui.setupUi(this);
	ui.logEd->document()->setHtml("");
	ui.logEd->document()->setMaximumBlockCount(2000);
	//ui.logEd->setFocus(Qt::OtherFocusReason);

	connect(this, SIGNAL(message_add(const QString &)), this, SLOT(add_message(const QString &)));
}

LogFrame::~LogFrame()
{

}

void LogFrame::emit_message_add(const QString &m) {
	emit message_add(m);
}

void LogFrame::add_message(const QString &m) {
	ui.logEd->append(m);
}

void LogFrame::showHideToggle() {
	if(isVisible()) hide();
	else show();
}

/////////////////////////////////////
PluginInfo info = {
	0x020,
	"Log Window",
	"Scott Ellis",
	"mail@scottellis.com.au",
	"http://www.scottellis.com.au",
	"Log Window",
	0x00000001
};

LogWindow::LogWindow(void)
{
}

LogWindow::~LogWindow(void)
{
}

bool LogWindow::load(CoreI *core) {
	core_i = core;

	log_frame = new LogFrame();
	g_log_frame = log_frame;

	oldHandler = qInstallMsgHandler(myMessageOutput);
	return true;
}

bool LogWindow::modules_loaded() {
	main_win_i = (MainWindowI *)core_i->get_interface(INAME_MAINWINDOW);
	if(main_win_i) main_win_i->add_window(log_frame);
	else log_frame->show();
	popup_i = (PopupI *)core_i->get_interface(INAME_POPUP);

	return true;
}

bool LogWindow::pre_shutdown() {
	qInstallMsgHandler(oldHandler);
	return true;
}

bool LogWindow::unload() {
	return true;
}

const PluginInfo &LogWindow::get_plugin_info() {
	return info;
}

Q_EXPORT_PLUGIN2(logWindow, LogWindow)
