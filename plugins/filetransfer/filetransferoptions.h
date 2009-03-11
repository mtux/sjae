#ifndef FILETRANSFEROPTIONS_H
#define FILETRANSFEROPTIONS_H

#include <options_i.h>
#include "ui_filetransferoptions.h"

class FileTransferOptions : public OptionsPageI
{
	Q_OBJECT

public:
	FileTransferOptions(QWidget *parent = 0);
	~FileTransferOptions();

	bool apply();
	void reset();

signals:
	void changed(bool valid = true);
	void applied();

private:
	Ui::FileTransferOptionsClass ui;
};

#endif // STYLESOPTIONS_H
