#include "searchwindow.h"
#include <QDebug>

SearchWindow::SearchWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.protoCmb, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(select_protocol(const QString &)));
	connect(ui.accountCmb, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(select_account(const QString &)));
}

SearchWindow::~SearchWindow() {
}

void SearchWindow::add_search_window(const QString &proto_name, ProtoSearchWindowI *search_window) {
	qDebug() << "adding search window for" << proto_name;
	accounts[proto_name].search_win = search_window;
	ui.stackedWidget->addWidget(search_window);
}

void SearchWindow::add_account(const QString &proto_name, const QString &id) {
	qDebug() << "adding account" << proto_name << id;
	if(!accounts.contains(proto_name) || !accounts[proto_name].accounts.contains(id)) {
		accounts[proto_name].accounts.append(id);
	}
	if(ui.protoCmb->findText(proto_name) == -1) {
		ui.protoCmb->addItem(proto_name);
		ui.protoCmb->setEnabled(true);
	}
}

void SearchWindow::remove_account(const QString &proto_name, const QString &id) {
	if(accounts.contains(proto_name)) {
		if(accounts[proto_name].accounts.contains(id)) {
			accounts[proto_name].accounts.removeAt(accounts[proto_name].accounts.indexOf(id));
			if(accounts[proto_name].accounts.size() == 0) {
				ui.protoCmb->removeItem(ui.protoCmb->findText(proto_name));
				if(ui.protoCmb->count() == 0) {
					ui.protoCmb->setEnabled(false);
					ui.accountCmb->setEnabled(false);
				}
			}
		}
	}
}


void SearchWindow::select_protocol(const QString &proto) {
	qDebug() << "protocol selected:" << proto;
	if(accounts.contains(proto)) {
		qDebug() << "setting stack widget to search window";
		ui.stackedWidget->setCurrentWidget(accounts[proto].search_win);

		ui.accountCmb->clear();
		ui.accountCmb->addItems(accounts[proto].accounts);
		ui.accountCmb->setCurrentIndex(0);
		ui.accountCmb->setEnabled(true);
	} else {
		qWarning() << "proto not known:" << proto << "known protos:" << accounts.keys();
	}
}

void SearchWindow::select_account(const QString &account) {
	if(!account.isEmpty()) {
		qDebug() << "account selected:" << account;
		QWidget *w = ui.stackedWidget->currentWidget();
		ProtoSearchWindowI *psw = (ProtoSearchWindowI *)w;
		if(psw)
			psw->set_account(account);
	}
}
