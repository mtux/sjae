#include "chatinput.h"
#include <QDebug>

ChatInput::ChatInput(QWidget *parent)
	: QTextEdit(parent)
{
	ui.setupUi(this);
}

ChatInput::~ChatInput()
{

}

void ChatInput::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Return && !(e->modifiers() & Qt::ShiftModifier)) {
		emit returnPressed();
		return;
	}

	QTextEdit::keyPressEvent(e);
}
