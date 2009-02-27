#ifndef GATEWAYREGISTER_H
#define GATEWAYREGISTER_H

#include <QDialog>
#include "ui_gatewayregister.h"
#include <QMap>
#include <QLineEdit>

class GatewayRegister : public QDialog
{
	Q_OBJECT

public:
	GatewayRegister(const QString &gateway, const QString &instructions, const QStringList &fields, QWidget *parent = 0);
	~GatewayRegister();
signals:
	void gatewayRegistration(const QString &gateway, const QMap<QString, QString> &fields);
protected slots:
	void reg();
private:
	Ui::GatewayRegisterClass ui;
	QStringList fieldNames;
	QMap<QString, QLineEdit *> field_map;
	QString gateway;
};

#endif // GATEWAYREGISTER_H
