#include "popupwin.h"
#include <QPalette>
#include <QDebug>

QRegion roundRectRegion(int x, int y, int w, int h, int radius) {
	QRegion r(x, y, w, h);
	// remove the corners
	r -= QRegion(x, y, radius, radius);
	r -= QRegion(x + w - radius, y, radius, radius);
	r -= QRegion(x + w - radius, y + h - radius, radius, radius);
	r -= QRegion(x, y + h - radius, radius, radius);
	// add rounded ones
	double daimeter = radius * 2;
	r += QRegion(x, y, daimeter, daimeter, QRegion::Ellipse);
	r += QRegion(x + w - daimeter, y, daimeter, daimeter, QRegion::Ellipse);
	r += QRegion(x + w - daimeter, y + h - daimeter, daimeter, daimeter, QRegion::Ellipse);
	r += QRegion(x, y + h - daimeter, daimeter, daimeter, QRegion::Ellipse);
	
	return r;
}

PopupWin::PopupWin(const PopupI::PopupClass &c, int i, bool round, QWidget *parent)
	: QWidget(parent), listener(c.listener), id(i), round_corners(round)
{
	ui.setupUi(this);

	setWindowFlags(Qt::Tool	| Qt::FramelessWindowHint| Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_DeleteOnClose, true);

	QPalette p = palette();
	p.setColor(QPalette::Background, c.background);
	setPalette(p);

	ui.lblSidebar->setPixmap(c.icon.pixmap(32, 32));

	p = ui.lblTitle->palette();
	p.setColor(QPalette::Foreground, c.title);
	ui.lblTitle->setPalette(p);

	p = ui.lblText->palette();
	p.setColor(QPalette::Foreground, c.text);
	ui.lblText->setPalette(p);

	if(c.timeout != 0) {
		closeTimer.setSingleShot(true);
		closeTimer.setInterval(c.timeout * 1000);
		connect(&closeTimer, SIGNAL(timeout()), this, SLOT(timeout()));
		closeTimer.start();
	}

	ui.lblText->installEventFilter(this);
}

PopupWin::~PopupWin()
{

}

bool PopupWin::eventFilter(QObject *obj, QEvent *e) {
	if(e->type() == QEvent::MouseButtonPress)
		mousePressEvent(static_cast<QMouseEvent *>(e));
	return QObject::eventFilter(obj, e);
}

void PopupWin::closeManual() {
	listener->popup_closed(id, PopupI::PDT_MANUAL);
	close();
}

void PopupWin::timeout() {
	listener->popup_closed(id, PopupI::PDT_TIMEOUT);
	close();
}

void PopupWin::closeEvent(QCloseEvent *e) {
	emit closed(id);
	QWidget::closeEvent(e);
}

// work around QLabel swallowing mouse press events when it contains rich text (e.g. links)
void PopupWin::mouseClose() {
	listener->popup_closed(id, mouseCloseReason);
	close();
}

void PopupWin::mousePressEvent(QMouseEvent *e) {
	if(e->button() == Qt::LeftButton)
		mouseCloseReason = PopupI::PDT_LEFT_CLICK;
	else if(e->button() == Qt::RightButton)
		mouseCloseReason = PopupI::PDT_RIGHT_CLICK;
	//e->accept();
	QTimer::singleShot(100, this, SLOT(mouseClose()));
}

void PopupWin::setIcon(const QIcon &icon) {
	ui.lblSidebar->setPixmap(icon.pixmap(64, 64));
}

void PopupWin::setContent(const QString &title, const QString &text) {
	ui.lblTitle->setText(QString("<b><big>%1</big></b>").arg(title));
	ui.lblText->setText(text);

	adjustSize();
	if(round_corners) {
		QRegion r = roundRectRegion(0, 0, width(), height(), 6);
		setMask(r);
	}
}
