#include "mainwin.h"
#include <QSettings>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QStyle>
#include <QToolButton>
#include <QDebug>
#include <QMenu>
#include <QResizeEvent>
#include <QRegion>
#include <QPaintEvent>
#include <QTimer>
#include <QDesktopWidget>

QRegion roundRectRegion(int x, int y, int w, int h, int radius) {
	QRegion r(x, y, w, h);
	// remove the corners
	r -= QRegion(x, y, radius, radius);
	r -= QRegion(x + w - radius, y, radius, radius);
	r -= QRegion(x + w - radius, y + h - radius, radius, radius);
	r -= QRegion(x, y + h - radius, radius, radius);
	// add rounded ones
        int daimeter = radius * 2;
	r += QRegion(x, y, daimeter, daimeter, QRegion::Ellipse);
	r += QRegion(x + w - daimeter, y, daimeter, daimeter, QRegion::Ellipse);
	r += QRegion(x + w - daimeter, y + h - daimeter, daimeter, daimeter, QRegion::Ellipse);
	r += QRegion(x, y + h - daimeter, daimeter, daimeter, QRegion::Ellipse);
	
	return r;
}

MainWin::MainWin(CoreI *core, QWidget *parent)
	: QMainWindow(parent), core_i(core), closing(false), mousePressed(false), 
		hideFrame(false), toolWindow(false), roundCorners(false), onTop(false)
{
	ui.setupUi(this);

	icons_i = (IconsI *)core_i->get_interface(INAME_ICONS);
	if(icons_i) setWindowIcon(icons_i->get_icon("generic"));
	menus_i = (MenusI *)core_i->get_interface(INAME_MENUS);

	QAction *exitAct;
	if(menus_i) {
		exitAct = menus_i->add_menu_action("Main Menu", tr("E&xit"), "quit");
		sepAction = menus_i->add_menu_separator("Main Menu", exitAct);
		winMenu = menus_i->get_menu("Main Menu");
	} else {
		winMenu = new QMenu("Main Menu", this);
		sepAction = winMenu->addSeparator();
		exitAct = new QAction(QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton), tr("E&xit"), this);
		winMenu->addAction(exitAct);
	}
	exitAct->setShortcut(tr("Ctrl+Q"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(quit()));

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
	systray->setToolTip("saje");
	systray->setContextMenu(winMenu);
	systray->show();

	connect(QApplication::desktop(), SIGNAL(workAreaResized(int)), this, SLOT(ensureOnScreen(int)));
}

MainWin::~MainWin()
{
	QSettings settings;
	settings.setValue("MainWin/geometry", saveGeometry());
	settings.setValue("MainWin/hiddenState", isHidden());
}

void MainWin::modules_loaded() {
}

void MainWin::ensureOnScreen(int screenChanged) {

	int myscreen = QApplication::desktop()->screenNumber(this);
	if(myscreen == -1) myscreen = QApplication::desktop()->primaryScreen();
	QRect screenGeom = QApplication::desktop()->availableGeometry(myscreen),
		myGeom = geometry();

	if(screenChanged == myscreen) {
		if(!screenGeom.contains(myGeom, true)) {
			if(myGeom.left() < screenGeom.left())
				myGeom.moveLeft(0);
			if(myGeom.right() > screenGeom.right())
				myGeom.moveRight(screenGeom.right());

			if(myGeom.top() < screenGeom.top())
				myGeom.moveTop(0);
			if(myGeom.bottom() > screenGeom.bottom())
				myGeom.moveBottom(screenGeom.bottom());

			setGeometry(myGeom);
		}
	}
}

void MainWin::updateFlags() {
	bool hidden = isHidden();
	setWindowFlags((toolWindow ? Qt::Tool : Qt::Window) 
		| (hideFrame ? Qt::FramelessWindowHint : Qt::Widget)
		| (onTop ? Qt::WindowStaysOnTopHint : Qt::Widget));

	if(!hidden) show();	
}

void MainWin::set_options(MainWinOptions::Settings settings) {
	ui.toolBar->setVisible(!settings.hide_toolbar);
	hideFrame = settings.hide_frame;
	toolWindow = settings.tool_window;
	setWindowOpacity(1 - (settings.trans_percent / 100.0));
	roundCorners = (settings.round_corners && hideFrame);
	if(roundCorners && hideFrame) {
		QRegion r = roundRectRegion(0, 0, width(), height(), 6);
		setMask(r);
	} else
		clearMask();
	onTop = settings.on_top;

	updateFlags();
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
		default:
			break;
	}
}

void MainWin::toggleHidden() {
	if(isHidden()) {
		show();
		activateWindow();
		raise();
	} else 
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
	if(menus_i) {
		if(icons_i) icons_i->add_icon(w->windowTitle(), w->windowIcon().pixmap(32, 32), w->windowTitle());
		action = menus_i->add_menu_action("Main Menu", w->windowTitle(), w->windowTitle(), sepAction);
	} else {
		action = new QAction(w->windowIcon(), w->windowTitle(), this);
		winMenu->insertAction(sepAction, action);
	}
	connect(action, SIGNAL(triggered()), w, SLOT(show()));
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

void MainWin::resizeEvent(QResizeEvent *e) {
	if(roundCorners) {
		QRegion r = roundRectRegion(0, 0, width(), height(), 10);
		setMask(r);
	}
}

void MainWin::paintEvent(QPaintEvent *e) {
	//qDebug() << "Widget rect" << rect();
	//qDebug() << "Paint rect:" << e->rect();
	QMainWindow::paintEvent(e);
}
