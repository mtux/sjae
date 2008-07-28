#include "mainwin.h"
#include <QSettings>

MainWin::MainWin(CoreI *core, QWidget *parent)
	: QMainWindow(parent), core_i(core)
{
	ui.setupUi(this);
	icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);

	if(icons_i) {
		setWindowIcon(icons_i->get_icon("generic"));
		winMenu = menuBar()->addMenu(QIcon(icons_i->get_icon("dot_grey_16")), "");
	} else 
		winMenu = menuBar()->addMenu("Main Menu");

	QSettings settings;
	restoreGeometry(settings.value(windowTitle() + "/geometry").toByteArray());
}

MainWin::~MainWin()
{
	QSettings settings;
	settings.setValue(windowTitle() + "/geometry", saveGeometry());
}

void MainWin::add_window(QWidget *w) {
	QSettings settings;
	w->restoreGeometry(settings.value(w->windowTitle() + "/geometry").toByteArray());
	w->installEventFilter(this);

	QAction *action = winMenu->addAction(w->windowTitle());
	connect(action, SIGNAL(triggered()), w, SLOT(show()));
}

bool MainWin::eventFilter(QObject *target, QEvent *event) {
	if(event->type() == QEvent::Close) {
		QWidget *w = (QWidget *)target;
		QSettings settings;
		settings.setValue(w->windowTitle() + "/geometry", w->saveGeometry());
	}

	return QMainWindow::eventFilter(target, event);
}

