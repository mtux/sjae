#ifndef CHATINPUT_H
#define CHATINPUT_H

#include <QTextEdit>
#include "ui_chatinput.h"
#include <QKeyEvent>

class ChatInput : public QTextEdit
{
	Q_OBJECT

public:
	ChatInput(QWidget *parent = 0);
	~ChatInput();

signals:
	void returnPressed();

protected:
	virtual void keyPressEvent(QKeyEvent *e);

private:
	Ui::ChatInputClass ui;
};

#endif // CHATINPUT_H
