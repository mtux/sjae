#include "smileysoptions.h"
#include <QInputDialog>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QIcon>

SmileysOptions::SmileysOptions(const Settings &s, QWidget *parent) :
	OptionsPageI(parent),
	m_ui(new Ui::SmileysOptions),
	current_settings(s)
{
    m_ui->setupUi(this);

	connect(m_ui->chkEnable, SIGNAL(clicked()), this, SIGNAL(changed()));

	reset();
}

SmileysOptions::~SmileysOptions()
{
    delete m_ui;
}

bool SmileysOptions::apply() {
	current_settings.subs = subs;
	current_settings.enable = m_ui->chkEnable->isChecked();
	emit applied();
	return true;
}

void SmileysOptions::reset() {
	m_ui->listWidget->clear();
	subs = current_settings.subs;
	foreach(QString sub, subs.keys()) {
		QListWidgetItem *item = new QListWidgetItem(sub, m_ui->listWidget);
		item->setData(Qt::DecorationRole, QIcon(subs[sub]));
	}
	m_ui->chkEnable->setChecked(current_settings.enable);
}

void SmileysOptions::on_btnAdd_clicked()
{
	QString sub = QInputDialog::getText(this, tr("Text"), tr("Text") + ":");
	if(!sub.isEmpty()) {
		QString fn = QFileDialog::getOpenFileName(this, tr("Image"), QApplication::applicationDirPath(), tr("Image Files (*.png *.jpg *.gif)"));
		if(!fn.isEmpty()) {
			subs[sub] = fn;
			QListWidgetItem *item = new QListWidgetItem(sub, m_ui->listWidget);
			item->setData(Qt::DecorationRole, QIcon(fn));
			emit changed();
		}
	}
}

void SmileysOptions::on_btnDel_clicked()
{
	QListWidgetItem *item = m_ui->listWidget->currentItem();
	if(item) {
		QString sub = item->text();
		subs.remove(sub);
		QListWidgetItem *item = m_ui->listWidget->takeItem(m_ui->listWidget->currentRow());
		delete item;
		m_ui->btnDel->setEnabled(false);
	}
}

void SmileysOptions::on_listWidget_itemSelectionChanged()
{
	if(m_ui->listWidget->currentRow() >= 0)
		m_ui->btnDel->setEnabled(true);
}
