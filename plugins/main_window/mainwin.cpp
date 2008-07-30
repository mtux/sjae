#include "mainwin.h"
#include <QSettings>
#include <QSystemTrayIcon>

MainWin::MainWin(CoreI *core, QWidget *parent)
	: QMainWindow(parent), core_i(core), closing(false)
{
	ui.setupUi(this);
	icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);

	if(icons_i) {
		setWindowIcon(icons_i->get_icon("generic"));
		winMenu = menuBar()->addMenu(QIcon(icons_i->get_icon("dot_grey_16")), "");
	} else 
		winMenu = menuBar()->addMenu("Main Menu");

	sepAction = winMenu->addSeparator();

	QAction *exitAct = new QAction(tr("E&xit"), this);
	exitAct->setShortcut(tr("Ctrl+Q"));
	//exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
	
	winMenu->addAction(exitAct);

	QSettings settings;
	restoreGeometry(settings.value("MainWin/geometry").toByteArray());

	systray = new QSystemTrayIcon(this);
	if(icons_i) systray->setIcon(icons_i->get_icon("generic"));
	connect(systray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systrayActivated(QSystemTrayIcon::ActivationReason)));
	systray->setContextMenu(winMenu);
	systray->show();
}

MainWin::~MainWin()
{
	QSettings settings;
	settings.setValue("MainWin/geometry", saveGeometry());
}

void MainWin::systrayActivated(QSystemTrayIcon::ActivationReason reason) {
	switch(reason) {
		case QSystemTrayIcon::Trigger:
			toggleHidden();
			break;
		case QSystemTrayIcon::Context:
			break;
	}
}

void MainWin::toggleHidden() {
	if(isHidden()) {
		show();
		activateWindow();
	} else 
		hide();
}

void MainWin::hideEvent(QHideEvent *e) {
	QSettings settings;
	if(!closing) settings.setValue("MainWin/hiddenState", true);
	QMainWindow::hideEvent(e);
}

void MainWin::showEvent(QShowEvent *e) {
	QSettings settings;
	settings.setValue("MainWin/hiddenState", false);
	QMainWindow::showEvent(e);
}

void MainWin::closeEvent(QCloseEvent *e) {
	closing = true;
	QMainWindow::closeEvent(e);
}

void MainWin::restoreHiddenState() {
	QSettings settings;
	bool hidden = settings.value("MainWin/hiddenState", false).toBool();
	if(hidden) hide();
	else show();
}

void MainWin::add_window(QWidget *w) {
	QSettings settings;
	w->restoreGeometry(settings.value(w->windowTitle() + "/geometry").toByteArray());
	w->installEventFilter(this);

	QAction *action;
	winMenu->insertAction(sepAction, action = new QAction(w->windowIcon(), w->windowTitle(), this));
	connect(action, SIGNAL(triggered()), w, SLOT(show()));
}

void MainWin::add_submenu(QMenu *menu) {
	winMenu->insertMenu(sepAction, menu);
}

bool MainWin::eventFilter(QObject *target, QEvent *e) {
	QSettings settings;
	if(e->type() == QEvent::Close) {
		QWidget *w = (QWidget *)target;
		settings.setValue(w->windowTitle() + "/geometry", w->saveGeometry());
	}

	return QMainWindow::eventFilter(target, e);
}

