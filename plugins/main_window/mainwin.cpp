#include "mainwin.h"
#include <QSettings>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QStyle>
#include <QToolButton>
#include <QDebug>
#include <QMenu>

MainWin::MainWin(CoreI *core, QWidget *parent)
	: QMainWindow(parent), core_i(core), closing(false), mousePressed(false), hideFrame(false), toolWindow(false)
{
	ui.setupUi(this);

	icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);
	winMenu = new QMenu("MainMenu", this);
	if(icons_i) {
		setWindowIcon(icons_i->get_icon("generic"));
		//winMenu = menuBar()->addMenu(QIcon(icons_i->get_icon("dot_grey")), "");
		winMenu->setIcon(icons_i->get_icon("dot_grey"));
	} //else 
		//winMenu = menuBar()->addMenu("Main Menu");

	sepAction = winMenu->addSeparator();

	QAction *exitAct = new QAction(QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton), tr("E&xit"), this);
	exitAct->setShortcut(tr("Ctrl+Q"));
	//exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(quit()));
	
	winMenu->addAction(exitAct);

	QToolButton *mainMenuButton = new QToolButton(this);
	mainMenuButton->setMenu(winMenu);
	mainMenuButton->setIcon(winMenu->icon());
	mainMenuButton->setPopupMode(QToolButton::InstantPopup);
	ui.toolBar->addWidget(mainMenuButton);
	ui.toolBar->addAction(exitAct);

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

void MainWin::updateFlags() {
	//Qt::WindowFlags flags = windowFlags();

	setWindowFlags((toolWindow ? Qt::Tool : Qt::Window) | (hideFrame ? Qt::FramelessWindowHint : Qt::Widget));
	show();	
}

void MainWin::set_hide_frame(bool hide) {
	hideFrame = hide;
	updateFlags();
}

void MainWin::set_tool_window(bool tool) {
	toolWindow = tool;
	updateFlags();
}

void MainWin::set_transparency(int trans_percent) {
	setWindowOpacity(1 - (trans_percent / 100.0));
}

void MainWin::quit() {
	closing = true;
	qApp->quit();
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
	e->setAccepted(false);
	hide();
}

void MainWin::restoreHiddenState() {
	QSettings settings;
	bool hidden = settings.value("MainWin/hiddenState", false).toBool();
	if(hidden) hide();
	else show();
}

void MainWin::manage_window_position(QWidget *w) {
	QSettings settings;
	w->restoreGeometry(settings.value(w->windowTitle() + "/geometry").toByteArray());
	w->installEventFilter(this);
}

void MainWin::add_window(QWidget *w) {
	manage_window_position(w);

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

void MainWin::mousePressEvent(QMouseEvent *e) {
	mousePressed = true;
	cursorOffset = e->globalPos() - pos();
	QMainWindow::mousePressEvent(e);
}

void MainWin::mouseMoveEvent(QMouseEvent *e) {
	if(mousePressed) {
		move(e->globalPos() - cursorOffset);
		//cursorOffset = e->globalPos();// - QWidget::mapToGlobal(pos());
	}
	QMainWindow::mouseMoveEvent(e);
}

void MainWin::mouseReleaseEvent(QMouseEvent *e) {
	mousePressed = false;
	QMainWindow::mouseReleaseEvent(e);
}
