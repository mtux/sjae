#include <QtGui/QApplication>
#include "core.h"

Core *core = 0;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QApplication::setOrganizationName("Scott Ellis");
	QApplication::setOrganizationDomain("scottellis.com.au");
	QApplication::setApplicationName("saje");
	QApplication::setQuitOnLastWindowClosed(false);

	core = new Core(&a);
	QObject::connect(&a, SIGNAL(aboutToQuit()), core, SLOT(unload_plugins()));

	return a.exec();
}
