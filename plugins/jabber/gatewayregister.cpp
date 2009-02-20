#include "gatewayregister.h"

GatewayRegister::GatewayRegister(const QString &gw, const QString &instructions, const QStringList &fieldList, const QStringList &valueList, QWidget *parent)
	: QDialog(parent), fieldNames(fieldList), values(valueList), gateway(gw)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	setWindowTitle(gateway);
	ui.lblInstructions->setText(instructions);

	QLineEdit *le;
	int index = 0;
	foreach(QString fname, fieldNames) {
		ui.formLayout->addRow(new QLabel(fname, this), le = new QLineEdit(this));
		field_map[fname] = le;
		le->setText(values.at(index));
		if(fname.compare("password", Qt::CaseInsensitive) == 0)
			le->setEchoMode(QLineEdit::Password);
		index++;
	}

	connect(this, SIGNAL(accepted()), this, SLOT(reg()));
}

GatewayRegister::~GatewayRegister()
{

}

void GatewayRegister::reg() {
	QMap<QString, QString> fields;
	foreach(QString fname, fieldNames) {
		fields[fname] = field_map[fname]->text();
	}
	emit gatewayRegistration(gateway, fields);
}
