#include "accountsoptions.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QDebug>

AccountsOptions::AccountsOptions(AccountsI *acc, QWidget *parent)
	: OptionsPageI(parent), accounts_i(acc)
{
	ui.setupUi(this);
	ui.edPort->setValidator(portValidator = new QIntValidator(0, 65535, this));
	reset();
}

AccountsOptions::~AccountsOptions()
{
	while(ui.stackedWidget->count() > 1) {
		QWidget *w = ui.stackedWidget->widget(ui.stackedWidget->count() - 1);
		ui.stackedWidget->removeWidget(w);
		disconnect(w, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
		delete w;
	}
}

bool AccountsOptions::isValid(const Account &account) {
	int pos = 0;
	return (account.host.isEmpty() == false
		//&& info.password.isEmpty() == false
		&& account.nick.isEmpty() == false
		&& account.username.isEmpty() == false
		&& account.proto != 0);
}

bool AccountsOptions::apply() {
	QList<Account *> deleted_accounts;
	QStringList proto_names = accounts_i->protocol_names();
	foreach(QString proto_name, proto_names) {
		QStringList account_ids = accounts_i->account_ids(proto_name);
		foreach(QString account_id, account_ids) {
			if(acc_info.contains(proto_name) == false || acc_info[proto_name].contains(account_id) == false)
				deleted_accounts.append(accounts_i->account_info(proto_name, account_id));
		}
	}

	{
		QListIterator<Account *> i(deleted_accounts);
		while(i.hasNext()) {
			Account *ap = i.next();
			ui.stackedWidget->removeWidget(account_extra_map[ap->proto->name()][ap->account_id]);
			accounts_i->remove_account(ap);
		}
	}

	{
		QMapIterator<QString, QMap<QString, Account> > i(acc_info);
		while(i.hasNext()) {
			i.next();
			QMapIterator<QString, Account> j(i.value());
			while(j.hasNext()) {
				j.next();
				Account *a = accounts_i->set_account_info(j.value());
				if(account_extra_map.contains(a->proto->name()) && account_extra_map[a->proto->name()].contains(a->account_id) && account_extra_map[a->proto->name()][a->account_id] != 0) {
					account_extra_map[a->proto->name()][a->account_id]->set_account_info(a);
					account_extra_map[a->proto->name()][a->account_id]->apply();
				}
			}
		}
	}

	emit applied();

	return true;
}

void AccountsOptions::enableAccountInfo(bool enable) {
	ui.cmbAccount->setEnabled(enable);
	ui.btnDel->setEnabled(enable);

	ui.edPass->setEnabled(enable);
	ui.edNick->setEnabled(enable);
	ui.edUname->setEnabled(enable);

	ProtocolI *proto = accounts_i->get_proto_interface(ui.cmbProto->currentText());
	if(proto) {
		ui.edHost->setEnabled(enable && proto->allowSetHost());
		ui.edPort->setEnabled(enable && proto->allowSetPort());
	} else {
		ui.edHost->setEnabled(false);
		ui.edPort->setEnabled(false);
	}

	foreach(QString proto_name, account_extra_map.keys()) {
		foreach(AccountExtra *w, account_extra_map[proto_name].values())
			w->setEnabled(enable);
	}
}

void AccountsOptions::reset() {
	bool enableWindow = true, enableInfo = true;
	
	while(ui.stackedWidget->count() > 1) {
		QWidget *w = ui.stackedWidget->widget(ui.stackedWidget->count() - 1);
		ui.stackedWidget->removeWidget(w);
		disconnect(w, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
		delete w;
	}
	account_extra_map.clear();
	acc_info.clear();

	QStringList proto_names = accounts_i->protocol_names();
	for(int i = 0; i < proto_names.size(); i++) {
		ProtocolI *pi = accounts_i->get_proto_interface(proto_names.at(i));

		QStringList acc_ids = accounts_i->account_ids(pi);
		for(int j = 0; j < acc_ids.size(); j++) {
			//qDebug() << "Added proto" << proto_names.at(i) << "account" << acc_ids.at(j);
			Account *acc = accounts_i->account_info(pi, acc_ids.at(j));
			acc_info[proto_names.at(i)][acc->account_id] = *acc;

			AccountExtra *w = pi->create_account_extra(acc);
			if(w) {
				connect(w, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
				account_extra_map[proto_names.at(i)][acc->account_id] = w;
				ui.stackedWidget->addWidget(w);
			} else 
				account_extra_map[proto_names.at(i)][acc->account_id] = 0;
		}
	}

	if(proto_names.size()) {
		QString sel = ui.cmbProto->currentText();
		ui.cmbProto->clear();
		ui.cmbProto->addItems(proto_names);
		if(proto_names.contains(sel))
			ui.cmbProto->setCurrentIndex(proto_names.indexOf(sel));
		else 
			ui.cmbProto->setCurrentIndex(0);
	} else {
		ui.cmbProto->clear();
		enableWindow = false;
	}

	if(enableWindow) {
		enableInfo = false;
		QString sel = ui.cmbAccount->currentText();
		ui.cmbAccount->clear();
		foreach(QString acc_id, acc_info[ui.cmbProto->currentText()].keys()) {
			enableInfo = true;
			ui.cmbAccount->addItem(acc_info[ui.cmbProto->currentText()][acc_id].account_name);
		}
		int index = 0;
		if((index = ui.cmbAccount->findText(sel)) == -1) index = 0;
		ui.cmbAccount->setCurrentIndex(index);
	} else
		ui.cmbAccount->clear();

	if(enableWindow && enableInfo) {
		setAccInfo(ui.cmbProto->currentText(), ui.cmbAccount->currentIndex());
	}

	enableAccountInfo(enableInfo);
	setEnabled(enableWindow);
}

void AccountsOptions::setAccInfo(const QString &proto, int acc_index) {
	if(acc_info.contains(proto) && acc_info[proto].size() > acc_index) {
		QString account_id = acc_info[proto].keys().at(acc_index);
		if(account_extra_map.contains(proto) && account_extra_map[proto].contains(account_id) && account_extra_map[proto][account_id] != 0)
			ui.stackedWidget->setCurrentWidget(account_extra_map[proto][account_id]);
		else
			ui.stackedWidget->setCurrentIndex(0);
		Account &account = acc_info[proto][account_id];

		ui.edHost->setText(account.host);
		ui.edPass->setText(account.password);
		ui.edPort->setText(QString("%1").arg(account.port));
		ui.edNick->setText(account.nick);
		ui.edUname->setText(account.username);

		ui.edHost->setEnabled(account.proto->allowSetHost());
		ui.edPort->setEnabled(account.proto->allowSetPort());
	}
}


void AccountsOptions::on_btnCreate_clicked() {
	bool ok;

	ProtocolI *proto = accounts_i->get_proto_interface(ui.cmbProto->currentText());
	// make new account
	QString account_id = QUuid::createUuid().toString();
	Account &info = acc_info[proto->name()][account_id];
	info.account_name = (proto->name() + " %1").arg(acc_info[proto->name()].size());
	info.account_id = account_id;
	info.proto = proto;
	info.host = proto->defaultHost();
	info.nick = QDir::home().dirName();
	info.port = proto->defaultPort();
	info.enabled = true;

	AccountExtra *w = proto->create_account_extra(0);
	if(w) {
		connect(w, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
		account_extra_map[proto->name()][info.account_id] = w;
		ui.stackedWidget->addWidget(w);
	} else 
		account_extra_map[proto->name()][info.account_id] = 0;

	QStringList acc_ids = acc_info[proto->name()].keys();
	ui.cmbAccount->clear();
	int i = 0, index = 0;
	foreach(QString acc_id, acc_info[ui.cmbProto->currentText()].keys()) {
		ui.cmbAccount->addItem(acc_info[ui.cmbProto->currentText()][acc_id].account_name);
		if(acc_id == info.account_id) {
			index = i;
			ui.cmbAccount->setCurrentIndex(i);
		}
		i++;
	}
	
	setAccInfo(proto->name(), index);
	enableAccountInfo(true);

	emit changed(false);
}

void AccountsOptions::checkValid() {
	bool valid = true;

	QMapIterator<QString, QMap<QString, Account> > i(acc_info);
	while(valid && i.hasNext()) {
		i.next();
		QMapIterator<QString, Account> j(i.value());
		while(valid && j.hasNext()) {
			j.next();
			valid = isValid(j.value()) && valid;
		}
	}
	emit changed(valid);
}

void AccountsOptions::on_btnDel_clicked() {
	acc_info[ui.cmbProto->currentText()].remove(acc_info[ui.cmbProto->currentText()].keys().at(ui.cmbAccount->currentIndex()));

	bool enableInfo = false;
	ui.cmbAccount->clear();
	foreach(QString acc_id, acc_info[ui.cmbProto->currentText()].keys()) {
		enableInfo = true;
		ui.cmbAccount->addItem(acc_info[ui.cmbProto->currentText()][acc_id].account_name);
	}
	if(enableInfo) {
		ui.cmbAccount->setCurrentIndex(0);
		setAccInfo(ui.cmbProto->currentText(), ui.cmbAccount->currentIndex());
	}

	enableAccountInfo(enableInfo);
	checkValid();
}

void AccountsOptions::on_edNick_textChanged(const QString &nick) { 
	QString proto_name = ui.cmbProto->currentText();
	int acc_index = ui.cmbAccount->currentIndex();
	if(acc_index >= 0 && acc_info.contains(proto_name)) {
		QString acc_id = acc_info[proto_name].keys().at(acc_index);
		if(acc_info[proto_name][acc_id].nick != nick) {
			acc_info[proto_name][acc_id].nick = nick;
			checkValid();
		}
	}
}

void AccountsOptions::on_edUname_textChanged(const QString &username) { 
	QString proto_name = ui.cmbProto->currentText();
	int acc_index = ui.cmbAccount->currentIndex();
	if(acc_index >= 0 && acc_info.contains(proto_name)) {
		QString acc_id = acc_info[proto_name].keys().at(acc_index);
		if(acc_info[proto_name][acc_id].username != username) {
			acc_info[proto_name][acc_id].username = username;
			checkValid();
		}
	}
}
void AccountsOptions::on_edPass_textChanged(const QString &pass) { 
	QString proto_name = ui.cmbProto->currentText();
	int acc_index = ui.cmbAccount->currentIndex();
	if(acc_index >= 0 && acc_info.contains(proto_name)) {
		QString acc_id = acc_info[proto_name].keys().at(acc_index);
		if(acc_info[proto_name][acc_id].password != pass) {
			acc_info[proto_name][acc_id].password = pass;
			checkValid();
		}
	}
}
void AccountsOptions::on_edHost_textChanged(const QString &host) { 
	QString proto_name = ui.cmbProto->currentText();
	int acc_index = ui.cmbAccount->currentIndex();
	if(acc_index >= 0 && acc_info.contains(proto_name)) {
		QString acc_id = acc_info[proto_name].keys().at(acc_index);
		if(acc_info[proto_name][acc_id].host != host) {
			acc_info[proto_name][acc_id].host = host;
			checkValid();
		}
	}
}
void AccountsOptions::on_edPort_textChanged(const QString &port) { 
	QString proto_name = ui.cmbProto->currentText();
	int acc_index = ui.cmbAccount->currentIndex();
	if(acc_index >= 0 && acc_info.contains(proto_name)) {
		QString acc_id = acc_info[proto_name].keys().at(acc_index);
		if(acc_info[proto_name][acc_id].port != port.toInt()) {
			acc_info[proto_name][acc_id].port = port.toInt();
			checkValid();
		}
	}
}

void AccountsOptions::on_cmbAccount_currentIndexChanged(int index) {
	if(index >= 0)
		setAccInfo(ui.cmbProto->currentText(), index);
}

void AccountsOptions::on_cmbAccount_editTextChanged(QString t)
{
	QString proto_name = ui.cmbProto->currentText();
	int acc_index = ui.cmbAccount->currentIndex();
	if(acc_index >= 0 && acc_info.contains(proto_name)) {
		QString acc_id = acc_info[proto_name].keys().at(acc_index);
		if(acc_info[proto_name][acc_id].account_name != t) {
			acc_info[proto_name][acc_id].account_name = t;
			checkValid();
		}
	}
}
