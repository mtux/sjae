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
		&& account.port >= 0
		&& account.port <= 65535
		&& account.nick.isEmpty() == false
		&& account.username.isEmpty() == false
		&& account.proto != 0);
}

bool AccountsOptions::apply() {
	QList<Account *> deleted_accounts;
	QStringList proto_names = accounts_i->protocol_names();
	foreach(QString proto_name, proto_names) {
		QStringList account_names = accounts_i->account_ids(proto_name);
		foreach(QString account_id, account_names) {
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
		QStringList acc_ids = acc_info[ui.cmbProto->currentText()].keys();
		if(acc_ids.size()) {
			QString sel = ui.cmbAccount->currentText();
			ui.cmbAccount->clear();
			ui.cmbAccount->addItems(acc_ids);
			if(acc_ids.contains(sel))
				ui.cmbAccount->setCurrentIndex(acc_ids.indexOf(sel));
			else
				ui.cmbAccount->setCurrentIndex(0);
		} else {
			ui.cmbAccount->clear();
			enableInfo = false;
		}
	} else
		ui.cmbAccount->clear();

	if(enableWindow && enableInfo) {
		setAccInfo(ui.cmbProto->currentText(), ui.cmbAccount->currentText());
	}

	enableAccountInfo(enableInfo);
	setEnabled(enableWindow);
}

void AccountsOptions::setAccInfo(const QString &proto, const QString &account_id) {
	
	if(acc_info.contains(proto) && acc_info[proto].contains(account_id)) {
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
	QString text = QInputDialog::getText(this, tr("New Account: ") + proto->name(), tr("Account name:"), QLineEdit::Normal, QDir::home().dirName(), &ok);
	if (ok && !text.isEmpty()) {
		QStringList account_names = acc_info[proto->name()].keys();
		if(account_names.contains(text)) {
			QMessageBox::information(this, tr("New Account"), tr("That account name is already in use"));
		} else {
			// make new account
			Account &info = acc_info[proto->name()][text];
			info.account_id = text;
			info.proto = proto;
			info.host = proto->defaultHost();
			info.nick = text;
			info.port = proto->defaultPort();
			info.enabled = true;

			AccountExtra *w = proto->create_account_extra(0);
			if(w) {
				connect(w, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
				account_extra_map[proto->name()][text] = w;
				ui.stackedWidget->addWidget(w);
			} else 
				account_extra_map[proto->name()][text] = 0;

			QStringList acc_ids = acc_info[proto->name()].keys();
			ui.cmbAccount->clear();
			ui.cmbAccount->addItems(acc_ids);
			ui.cmbAccount->setCurrentIndex(acc_ids.indexOf(text));

			setAccInfo(proto->name(), text);
			enableAccountInfo(true);

			emit changed(false);
		}
	}
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
	acc_info[ui.cmbProto->currentText()].remove(ui.cmbAccount->currentText());

	QStringList acc_ids = acc_info[ui.cmbProto->currentText()].keys();
	if(acc_ids.size()) {
		ui.cmbAccount->clear();
		ui.cmbAccount->addItems(acc_ids);
		ui.cmbAccount->setCurrentIndex(0);
		enableAccountInfo(true);

		setAccInfo(ui.cmbProto->currentText(), ui.cmbAccount->currentText());
	} else {
		ui.cmbAccount->clear();
		enableAccountInfo(false);
	}

	checkValid();
}

void AccountsOptions::on_edNick_textChanged(const QString &nick) { 
	if(acc_info.contains(ui.cmbProto->currentText()) && acc_info[ui.cmbProto->currentText()].contains(ui.cmbAccount->currentText())) {
		if(acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].nick != nick) {
			acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].nick = nick;
			checkValid();
		}
	}
}

void AccountsOptions::on_edUname_textChanged(const QString &username) { 
	if(acc_info.contains(ui.cmbProto->currentText()) && acc_info[ui.cmbProto->currentText()].contains(ui.cmbAccount->currentText())) {
		if(acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].username != username) {
			acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].username = username;
			checkValid();
		}
	}
}
void AccountsOptions::on_edPass_textChanged(const QString &pass) { 
	if(acc_info.contains(ui.cmbProto->currentText()) && acc_info[ui.cmbProto->currentText()].contains(ui.cmbAccount->currentText())) {
		if(acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].password != pass) {
			acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].password = pass;
			checkValid();
		}
	}
}
void AccountsOptions::on_edHost_textChanged(const QString &host) { 
	if(acc_info.contains(ui.cmbProto->currentText()) && acc_info[ui.cmbProto->currentText()].contains(ui.cmbAccount->currentText())) {
		if(acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].host != host) {
			acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].host = host;
			checkValid();
		}
	}
}
void AccountsOptions::on_edPort_textChanged(const QString &port) { 
	if(acc_info.contains(ui.cmbProto->currentText()) && acc_info[ui.cmbProto->currentText()].contains(ui.cmbAccount->currentText())) {
		if(acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].port != port.toInt()) {
			acc_info[ui.cmbProto->currentText()][ui.cmbAccount->currentText()].port = port.toInt();
			checkValid();
		}
	}
}

void AccountsOptions::on_cmbAccount_currentIndexChanged(QString id) {
	if(!id.isEmpty())
		setAccInfo(ui.cmbProto->currentText(), id);
}