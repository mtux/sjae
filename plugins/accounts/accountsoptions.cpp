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

bool AccountsOptions::isValid(const AccountInfo &info) {
	int pos = 0;
	return (info.host.isEmpty() == false
		&& info.password.isEmpty() == false
		&& info.port >= 0
		&& info.port <= 65535
		&& info.nick.isEmpty() == false
		&& info.username.isEmpty() == false
		&& info.proto != 0);
}

bool AccountsOptions::apply() {
	{
		QMapIterator<QString, QList<QString> > i(deleted_ids);
		while(i.hasNext()) {
			i.next();
			QListIterator<QString> j(i.value());
			while(j.hasNext()) {
				accounts_i->remove_account(i.key(), j.next());
			}
		}
		deleted_ids.clear();
	}

	{
		QMapIterator<QString, QMap<QString, AccountInfo> > i(acc_info);
		while(i.hasNext()) {
			i.next();
			QMapIterator<QString, AccountInfo> j(i.value());
			while(j.hasNext()) {
				j.next();
				accounts_i->set_account_info(j.key(), j.value());
			}
		}
	}

	for(int i = 1; i < ui.stackedWidget->count(); i++) {
		static_cast<AccountExtra *>(ui.stackedWidget->widget(i))->apply();
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
	proto_extra_map.clear();
	acc_info.clear();
	deleted_ids.clear();

	QStringList proto_names = accounts_i->protocol_names();
	for(int i = 0; i < proto_names.size(); i++) {
		ProtocolI *pi = accounts_i->get_proto_interface(proto_names.at(i));

		QStringList acc_ids = accounts_i->account_ids(proto_names.at(i));
		for(int j = 0; j < acc_ids.size(); j++) {
			//qDebug() << "Added proto" << proto_names.at(i) << "account" << acc_ids.at(j);
			acc_info[proto_names.at(i)][acc_ids.at(j)] = accounts_i->account_info(proto_names.at(i), acc_ids.at(j));

			AccountExtra *w = pi->create_account_extra(acc_ids.at(j));
			if(w) {
				connect(w, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
				proto_extra_map[proto_names.at(i)][acc_ids.at(j)] = ui.stackedWidget->addWidget(w);
				w->set_info(acc_info[proto_names.at(i)][acc_ids.at(j)]);
			} else 
				proto_extra_map[proto_names.at(i)][acc_ids.at(j)] = 0;
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

void AccountsOptions::setAccInfo(const QString &proto, const QString &acc) {
	if(proto_extra_map.contains(proto) && proto_extra_map[proto].contains(acc))
		ui.stackedWidget->setCurrentIndex(proto_extra_map[proto][acc]);
	if(acc_info.contains(proto) && acc_info[proto].contains(acc)) {
		AccountInfo &info = acc_info[proto][acc];

		ui.edHost->setText(info.host);
		ui.edPass->setText(info.password);
		ui.edPort->setText(QString("%1").arg(info.port));
		ui.edNick->setText(info.nick);
		ui.edUname->setText(info.username);

		ui.edHost->setEnabled(info.proto->allowSetHost());
		ui.edPort->setEnabled(info.proto->allowSetPort());
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
			AccountInfo info;
			info.host = proto->defaultHost();
			info.nick = text;
			info.port = proto->defaultPort();
			info.proto = proto;
			info.enabled = true;

			acc_info[proto->name()][text] = info;
			proto->update_account_data(text, info);

			AccountExtra *w = proto->create_account_extra(text);
			if(w) {
				connect(w, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
				proto_extra_map[proto->name()][text] = ui.stackedWidget->addWidget(w);
				w->set_info(acc_info[proto->name()][text]);
			} else 
				proto_extra_map[proto->name()][text] = 0;

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

	QMapIterator<QString, QMap<QString, AccountInfo> > i(acc_info);
	while(valid && i.hasNext()) {
		i.next();
		QMapIterator<QString, AccountInfo> j(i.value());
		while(valid && j.hasNext()) {
			j.next();
			valid = isValid(j.value()) && valid;
		}
	}
	emit changed(valid);
}

void AccountsOptions::on_btnDel_clicked() {
	deleted_ids[ui.cmbProto->currentText()].append(ui.cmbAccount->currentText());
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