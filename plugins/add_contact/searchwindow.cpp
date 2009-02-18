#include "searchwindow.h"
#include <QDebug>

SearchWindow::SearchWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.protoCmb, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(select_protocol(const QString &)));
	connect(ui.accountCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(select_account(int)));
}

SearchWindow::~SearchWindow() {
}

bool SearchWindow::has_account(Account *acc) {
	return (accounts.contains(acc->proto->name()) && accounts[acc->proto->name()].accounts.contains(acc));
}

void SearchWindow::add_search_window(const QString &proto_name, ProtoSearchWindowI *search_window) {
	//qDebug() << "adding search window for" << proto_name;
	accounts[proto_name].search_win = search_window;
	ui.stackedWidget->addWidget(search_window);
}

void SearchWindow::add_account(Account *acc) {
	//qDebug() << "adding account" << proto_name << id;
	QString proto_name = acc->proto->name();
	if(!accounts.contains(proto_name) || !accounts[proto_name].accounts.contains(acc)) {
		accounts[proto_name].accounts.append(acc);
	}
	if(ui.protoCmb->findText(proto_name) == -1) {
		ui.protoCmb->addItem(proto_name);
		ui.protoCmb->setEnabled(true);
		if(ui.protoCmb->currentIndex() == -1)
			ui.protoCmb->setCurrentIndex(0);
	}
	select_protocol(ui.protoCmb->currentText());
}

void SearchWindow::remove_account(Account *acc) {
	QString proto_name = acc->proto->name();
	if(accounts.contains(proto_name)) {
		if(accounts[proto_name].accounts.contains(acc)) {
			accounts[proto_name].accounts.removeAt(accounts[proto_name].accounts.indexOf(acc));
			if(accounts[proto_name].accounts.size() == 0) {
				ui.protoCmb->removeItem(ui.protoCmb->findText(proto_name));
				ui.stackedWidget->removeWidget(accounts[proto_name].search_win);
				if(ui.protoCmb->count() == 0) {
					ui.protoCmb->setEnabled(false);
					ui.accountCmb->setEnabled(false);
				}
			}
		}
	}
}


void SearchWindow::select_protocol(const QString &proto) {
	//qDebug() << "protocol selected:" << proto;
	if(accounts.contains(proto)) {
		//qDebug() << "setting stack widget to search window";
		ui.stackedWidget->setCurrentWidget(accounts[proto].search_win);

		ui.accountCmb->clear();
		foreach(Account *acc, accounts[proto].accounts)
			ui.accountCmb->addItem(acc->account_name);
		ui.accountCmb->setCurrentIndex(0);
		ui.accountCmb->setEnabled(true);
	} else {
		qWarning() << "proto not known:" << proto << "known protos:" << accounts.keys();
	}
}

void SearchWindow::select_account(int index) {
	if(index >= 0) {
		//qDebug() << "account selected:" << account;
		QWidget *w = ui.stackedWidget->currentWidget();
		ProtoSearchWindowI *psw = (ProtoSearchWindowI *)w;
		if(psw)
			psw->set_account(accounts[ui.protoCmb->currentText()].accounts.at(index)->account_id);
	}
}
