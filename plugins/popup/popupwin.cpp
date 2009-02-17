#include "popupwin.h"
#include <QPalette>

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

PopupWin::PopupWin(const PopupI::PopupClass &c, int i, bool round_corners, QWidget *parent)
	: QWidget(parent), listener(c.listener), id(i)
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

	if(round_corners) {
		QRegion r = roundRectRegion(0, 0, width(), height(), 6);
		setMask(r);
	}
}

PopupWin::~PopupWin()
{

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
	QWidget::closeEvent(e);
	emit closed(id);
}

void PopupWin::mousePressEvent(QMouseEvent *e) {
	QWidget::mousePressEvent(e);
	if(e->button() == Qt::LeftButton)
		listener->popup_closed(id, PopupI::PDT_LEFT_CLICK);
	else if(e->button() == Qt::RightButton)
		listener->popup_closed(id, PopupI::PDT_RIGHT_CLICK);
	close();
}

void PopupWin::setIcon(const QIcon &icon) {
	ui.lblSidebar->setPixmap(icon.pixmap(64, 64));
}

void PopupWin::setContent(const QString &title, const QString &text) {
	ui.lblTitle->setText(QString("<b><big>%1</big></b>").arg(title));
	ui.lblText->setText(text);
}
