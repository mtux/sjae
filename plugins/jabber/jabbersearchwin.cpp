#include "jabbersearchwin.h"

JabberSearchWin::JabberSearchWin(JabberProto *jabber_proto, QWidget *parent)
	: ProtoSearchWindowI(parent), proto(jabber_proto)
{
	ui.setupUi(this);
}

JabberSearchWin::~JabberSearchWin()
{

}

void JabberSearchWin::set_account(const QString &id) {
	account = id;
}


void JabberSearchWin::on_addBtn_clicked()
{
	proto->add_contact(account, ui.jidEd->text());
}
