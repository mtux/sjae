#include <QtGui/QApplication>
#include "core.h"
#include <QSettings>

Core *core = 0;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QApplication::setOrganizationDomain("scottellis.com.au");
	QApplication::setOrganizationName("Saje");
	QApplication::setQuitOnLastWindowClosed(false);
	QApplication::setApplicationName("Saje");

	QSettings::setDefaultFormat(QSettings::IniFormat);

	core = new Core(&a);

	int ret = a.exec();

	delete core;
	return ret;
}
