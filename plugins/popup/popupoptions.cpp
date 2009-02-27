#include "popupoptions.h"
#include <QPalette>
#include <QColorDialog>
#include <QFileDialog>

PopupOptions::PopupOptions(PopupI *i, QWidget *parent)
	: OptionsPageI(parent), popup_i(i)
{
	ui.setupUi(this);

	connect(ui.chkEnable, SIGNAL(clicked()), this, SIGNAL(changed()));
	connect(ui.chkRoundCorners, SIGNAL(clicked()), this, SIGNAL(changed()));
}

PopupOptions::~PopupOptions()
{

}

bool PopupOptions::apply() {
	current_settings.enabled = ui.chkEnable->isChecked();
	current_settings.round_corners = ui.chkRoundCorners->isChecked();
	emit applied();
	on_lstClasses_currentRowChanged(ui.lstClasses->currentRow());
	return true;
}

void PopupOptions::reset() {
	ui.chkEnable->setChecked(current_settings.enabled);
	ui.chkRoundCorners->setChecked(current_settings.round_corners);

	ui.lstClasses->clear();
	foreach(PopupI::PopupClass c, current_settings.classes) {
		ui.lstClasses->addItem(c.name);
	}

	ui.lstClasses->setCurrentRow(0, QItemSelectionModel::Select);
}


void PopupOptions::on_lstClasses_currentRowChanged(int index)
{
	if(index != -1) {
		PopupI::PopupClass c = current_settings.classes[ui.lstClasses->item(index)->text()];
		ui.spnTimeout->setValue(c.timeout);

		QPalette p = ui.widTitleCol->palette();
		p.setColor(QPalette::Background, c.title);
		ui.widTitleCol->setPalette(p);

		p = ui.widTextCol->palette();
		p.setColor(QPalette::Background, c.text);
		ui.widTextCol->setPalette(p);

		p = ui.widBgCol->palette();
		p.setColor(QPalette::Background, c.background);
		ui.widBgCol->setPalette(p);

		ui.btnIcon->setIcon(c.icon);
	}
}

void PopupOptions::on_btnBgCol_clicked()
{
	PopupI::PopupClass c = current_settings.classes[ui.lstClasses->currentItem()->text()];
	QColor col = QColorDialog::getColor(c.background, this);
	current_settings.classes[ui.lstClasses->currentItem()->text()].background = col;

	QPalette p = ui.widBgCol->palette();
	p.setColor(QPalette::Background, col);
	ui.widBgCol->setPalette(p);

	emit changed();
}

void PopupOptions::on_btnTitleCol_clicked()
{
	PopupI::PopupClass c = current_settings.classes[ui.lstClasses->currentItem()->text()];
	QColor col = QColorDialog::getColor(c.title, this);
	current_settings.classes[ui.lstClasses->currentItem()->text()].title = col;

	QPalette p = ui.widTitleCol->palette();
	p.setColor(QPalette::Background, col);
	ui.widTitleCol->setPalette(p);

	emit changed();
}

void PopupOptions::on_btnTextCol_clicked()
{
	PopupI::PopupClass c = current_settings.classes[ui.lstClasses->currentItem()->text()];
	QColor col = QColorDialog::getColor(c.text, this);
	current_settings.classes[ui.lstClasses->currentItem()->text()].text = col;

	QPalette p = ui.widTextCol->palette();
	p.setColor(QPalette::Background, col);
	ui.widTextCol->setPalette(p);

	emit changed();
}

void PopupOptions::on_btnPreview_clicked()
{
	PopupI::PopupClass c = current_settings.classes[ui.lstClasses->currentItem()->text()];
	popup_i->show_custom(c, "Preview", "How do I look?", ui.chkRoundCorners->isChecked());
	popup_i->show_custom(c, "Preview", "This is what happens when there's a lot of text in the popup. And I do mean a lot.\n"
		"Here then I have an opportunity to either fill this space with a lot of useless junk, or try to say something that "
		"I feel is important and that may be of use to somebody. I would choose the latter, except that in my brief 34 years "
		"I haven't learned anything that important :) I guess if there's something I'd like my kids to beleive, it's that there's "
		"always a counterexampe; there's an exception to every rule. A little less certainty could go a long way. If more people beleived that, "
		"it would lead to a little less selfishness, and a little less oppression.", 
			ui.chkRoundCorners->isChecked());
	popup_i->show_custom(c, "Preview with a Really Really Long, Long, Long Title", "How do I look?", ui.chkRoundCorners->isChecked());
}

void PopupOptions::on_btnIcon_clicked()
{
	QString fn = QFileDialog::getOpenFileName(this, tr("Choose Icon"), "", tr("Image Files (*.png *.jpg *.bmp)"));
	if(!fn.isEmpty()) {
		current_settings.classes[ui.lstClasses->currentItem()->text()].icon = QIcon(fn);
		ui.btnIcon->setIcon(current_settings.classes[ui.lstClasses->currentItem()->text()].icon);
	
		emit changed();
	}
}

void PopupOptions::on_spnTimeout_valueChanged(int val)
{
	if(current_settings.classes[ui.lstClasses->currentItem()->text()].timeout != val) {
		current_settings.classes[ui.lstClasses->currentItem()->text()].timeout = val;
		emit changed();
	}
}
