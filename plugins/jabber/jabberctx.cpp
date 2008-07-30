#include "jabberctx.h"

#include <QtEndian>
#include <QByteArray>
#include <QDataStream>
#include <QThread>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QHostInfo>
#include <QTextCodec>
#include <QCryptographicHash>
#include <QDateTime>

#include "newrosteritemdialog.h"

#include <clist_i.h>

JabberCtx::JabberCtx(const QString &id, const AccountInfo &ai, CoreI *core, QObject *parent)
	: QObject(parent), account_id(id), acc_info(ai), useSSL(false), core_i(core), sstate(SSNONE), writer(&sendBuffer), sessionRequired(false), tlsAvailable(false), tlsRequired(false), currentStatus(ST_OFFLINE)
{
	sendBuffer.open(QIODevice::WriteOnly);

	connect(&sslSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
	connect(&sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors(const QList<QSslError> &)));
	connect(&sslSocket, SIGNAL(readyRead()), this, SLOT(readSocket()), Qt::QueuedConnection);
	connect(&sslSocket, SIGNAL(connected()), this, SLOT(socketConnected()));
	connect(&sslSocket, SIGNAL(encrypted()), this, SLOT(socketEncrypted()));
	connect(&sslSocket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
	connect(&sslSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));


	clist_i = (CListI *)core_i->get_interface(INAME_CLIST);
	if(clist_i) {
		//newRosterItemAction = clist_i->add_contact_action("Jabber", account_id, "Add...");
		//connect(newRosterItemAction, SIGNAL(triggered()), this, SLOT(addRosterItem()));

		editRosterItemAction = clist_i->add_contact_action("Jabber", account_id, "Edit...");
		connect(editRosterItemAction, SIGNAL(triggered()), this, SLOT(editRosterItem()));

		removeRosterItemAction = clist_i->add_contact_action("Jabber", account_id, "Remove");
		connect(removeRosterItemAction, SIGNAL(triggered()), this, SLOT(removeRosterItem()));


		grantAction = clist_i->add_contact_action("Jabber", account_id, "Grant");
		connect(grantAction, SIGNAL(triggered()), this, SLOT(grantSubscription()));

		revokeAction = clist_i->add_contact_action("Jabber", account_id, "Revoke");
		connect(revokeAction, SIGNAL(triggered()), this, SLOT(revokeSubscription()));

		requestAction = clist_i->add_contact_action("Jabber", account_id, "Request");
		connect(requestAction, SIGNAL(triggered()), this, SLOT(requestSubscription()));

		//	void aboutToShowMenu(const QString &proto_name, const QString &account_id, const QString &id);

		connect(clist_i, SIGNAL(aboutToShowContactMenu(const QString &, const QString &, const QString &)), this, SLOT(aboutToShowContactMenu(const QString &, const QString &, const QString &)));
		connect(clist_i, SIGNAL(aboutToShowGroupMenu(const QString &, const QString &, const QString &)), this, SLOT(aboutToShowGroupMenu(const QString &, const QString &, const QString &)));
	}
}

void JabberCtx::setAccountInfo(const AccountInfo &info) {
	acc_info = info;
}

JabberCtx::~JabberCtx()
{
	requestStatus(ST_OFFLINE);
}

void JabberCtx::aboutToShowContactMenu(const QString &proto_name, const QString &aid, const QString &id) {
	bool vis = (proto_name == "Jabber" && aid == account_id);
	//newRosterItemAction->setVisible(vis);

	removeRosterItemAction->setVisible(vis);
	editRosterItemAction->setVisible(vis);

	if(vis) {
		RosterItem *item = roster.get_item(id);
		SubscriptionType sub = item->getSubscription();

		bool to = (sub == ST_BOTH || sub == ST_TO),
			from = (sub == ST_BOTH || sub == ST_FROM);

		grantAction->setVisible(vis && !to);
		revokeAction->setVisible(vis && to);

		requestAction->setVisible(vis && !from);

		mid = id;
	} else {
		grantAction->setVisible(false);
		revokeAction->setVisible(false);

		requestAction->setVisible(false);
	}
}

void JabberCtx::aboutToShowGroupMenu(const QString &proto_name, const QString &account_id, const QString &full_gn) {
	removeRosterItemAction->setVisible(false);
	editRosterItemAction->setVisible(false);
	grantAction->setVisible(false);
	revokeAction->setVisible(false);
	requestAction->setVisible(false);
}

QString JabberCtx::to_clist_id(const QString &jid) {
	return "Jabber::" + account_id + "::" + jid;
}

bool JabberCtx::is_ctx_id(const QString &clist_id) {
	QStringList sl = clist_id.split("::");
	return sl.at(0) == "Jabber" && sl.at(1) == account_id;
}

QString JabberCtx::to_jid(const QString &clist_id) {
	QStringList sl = clist_id.split("::");
	return sl.at(2);
}

void JabberCtx::showMessage(const QString &message) {
	qDebug() << "Jabber message:" << message;
	//QMessageBox::information(0, tr("Jabber Message"), message);
}

void JabberCtx::log(const QString &message, LogMessageType type) {
	switch(type) {
		case LMT_NORMAL:
			qDebug() << "Jabber:" << message;
			break;
		case LMT_SEND:
				if(message.startsWith("<response xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\">"))
					qDebug() << "Jabber (send): Challenge response: CENSORED FOR SECURITY REASONS";
				else
					qDebug() << "Jabber (send):" << message;
			break;
		case LMT_RECV:
			qDebug() << "Jabber (recv):" << message;
			break;
		case LMT_WARNING:
			qWarning() << "Jabber:" << message;
			break;
		case LMT_ERROR:
			qCritical() << "Jabber:" << message;
			break;
	}
}

void JabberCtx::requestStatus(GlobalStatus gs) {
	if(gs == currentStatus) return;

	if(sstate == SSOK) {
		if(gs == ST_OFFLINE)
			connectToServer(false);
		else
			setStatus(gs);
	} else if(sstate == SSNONE && gs != ST_OFFLINE) {
		connectStatus = gs;
		connectToServer(true);
	} else {
		qWarning() << "Jabber account" << account_id << "status change request while in invalid state - ignored";
	}
}

void JabberCtx::setStatus(GlobalStatus gs) {
	currentStatus = gs;
	emit statusChanged(account_id, gs);
	if(sstate == SSOK)
		sendPresence();
}

void JabberCtx::changeSessionState(const SessionState &newState) {
	sstate = newState;
	switch(newState) {
		case SSNONE:
			setStatus(ST_OFFLINE);
			showMessage("Disconnected");
			sslSocket.close();
			//newRosterItemAction->setEnabled(false);
			setAllOffline();
			break;
		case SSSTARTSSL:
			setStatus(ST_CONNECTING);
			showMessage("Connecting...");
			sslSocket.connectToHostEncrypted(connectionHost.isEmpty() ? acc_info.host : connectionHost, acc_info.port);
			break;
		case SSSTARTTLS:
			setStatus(ST_CONNECTING);
			showMessage("Connecting...");
			sslSocket.connectToHost(connectionHost.isEmpty() ? acc_info.host : connectionHost, acc_info.port);
			break;
		case SSINITIALIZING:
			setStatus(ST_CONNECTING);
			showMessage("Initializing...");
			startStream();
			break;
		case SSAUTHORIZING:
			setStatus(ST_CONNECTING);
			showMessage("Authorizing...");
			authenticate();
			break;
		case SSLOGIN:
			setStatus(ST_CONNECTING);
			showMessage("Getting roster...");
			startStream();
			break;
		case SSOK:
			setStatus(connectStatus);
			showMessage("Ok");
			//newRosterItemAction->setEnabled(true);
			break;
		case SSTERMINATING:
			setStatus(ST_OFFLINE);
			showMessage("Disconnecting...");
			endStream();
			break;
	}
}
void JabberCtx::sendWriteBuffer() {
	sendBuffer.close();
	sendBuffer.open(QIODevice::ReadOnly);
	const QByteArray &b = sendBuffer.readAll();
	if(b.size()) {
		log(QString().append(b), LMT_SEND);
		sslSocket.write(b);
	}
	sendBuffer.close();
	sendBuffer.buffer().clear();
	sslSocket.flush();
	sendBuffer.open(QIODevice::WriteOnly);
}

void JabberCtx::socketError(QAbstractSocket::SocketError socketError) {
	log("Socket error:" + sslSocket.errorString(), LMT_ERROR);
	changeSessionState(SSNONE);
}

void JabberCtx::sslErrors(const QList<QSslError> &errors) {
	for(int i = 0; i < errors.size(); i++) {
		log("SSL error: " + errors.at(i).errorString(), LMT_WARNING);
	}
	sslSocket.ignoreSslErrors();
}

void JabberCtx::socketStateChanged(QAbstractSocket::SocketState socketState) {
	/*
	QString state = "Unknown";
	switch(socketState) {
		case QAbstractSocket::UnconnectedState: state = "Unconnected"; break;
		case QAbstractSocket::HostLookupState: state = "HostLookup"; break;
		case QAbstractSocket::ConnectingState: state = "Connecting"; break;
		case QAbstractSocket::ConnectedState: state = "Connected"; break;
		case QAbstractSocket::BoundState: state = "Bound"; break;
		case QAbstractSocket::ListeningState: state = "Listening"; break;
		case QAbstractSocket::ClosingState: state = "Closing"; break;
	}

	log("Socket state changed to: " + state);
	*/
}

void JabberCtx::readSocket() {
	
	if(sslSocket.bytesAvailable() == 0)
		return;

	QByteArray data = sslSocket.read(sslSocket.bytesAvailable());
	log(QString().append(data), LMT_RECV);

	reader.addData(data);
	while(!reader.atEnd()) {
		reader.readNext();
		if(reader.isStartElement()) {
			if(reader.name() == "stream") {
				parseStreamStart();
			} else if(reader.name() == "features") {
				parseFeatures();
			} else if(reader.name() == "challenge") {
				parseChallenge();
			} else if(reader.name() == "error") {
				parseError();
			} else if(reader.name() == "failure") {
				parseFailure();
			}else if(reader.name() == "success") {
				parseSuccess();
			}else if(reader.name() == "proceed") {
				parseProceed();
			}else if(reader.name() == "iq") {
				parseIq();
			}else if(reader.name() == "presence") {
				parsePresence();
			}else if(reader.name() == "message") {
				parseMessage();
			}
		}
		//log(QString("%1 - %2 / %3").arg(reader.tokenString()).arg(reader.name().toString()).arg(reader.namespaceUri().toString()));
	}
	if(reader.hasError() && reader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
		log("Reader error: " + reader.errorString());
	}
	
	//log(QString().append();
}

void JabberCtx::socketConnected() {
	log("Connected.");
	if(!useSSL) startStream();
}

void JabberCtx::socketDisconnected() {
	log("Disconnected.");
	changeSessionState(SSNONE);
}

void JabberCtx::socketEncrypted() {
	log("Encrypted.");
	changeSessionState(SSINITIALIZING);
}

void JabberCtx::connectToServer(bool con) {
	if(con && !sslSocket.isOpen()) {
		log("Connecting to " + (connectionHost.isEmpty() ? acc_info.host : connectionHost) + "...");
		if(useSSL) changeSessionState(SSSTARTSSL);
		else changeSessionState(SSSTARTTLS);
	} else if(!con && sslSocket.isOpen()) {
		log("Disconnecting...");
		changeSessionState(SSTERMINATING);
	}
}

void JabberCtx::startStream() {
	log("Initializing XML stream...");
	reader.clear();
	writer.writeStartDocument("1.0");
	writer.writeStartElement("stream:stream");
	writer.writeDefaultNamespace("jabber:client");
	writer.writeNamespace("http://etherx.jabber.org/streams", "stream");

	writer.writeAttribute("xml:lang", "en" );
	writer.writeAttribute("version", "1.0" );
	writer.writeAttribute("to", acc_info.host);
	writer.writeCharacters("");// to append '>' to the start element and leave it open
	sendWriteBuffer();
}

void JabberCtx::endStream() {
	writer.writeEndDocument();
	sendWriteBuffer();
	sslSocket.disconnectFromHost();
	log("XML stream terminated.");
	sslSocket.waitForDisconnected();
}

void JabberCtx::parseError() {
	reader.readNext();
	while(!reader.atEnd() && reader.name() != "error") {
		if(reader.isStartElement()) {
			if(reader.name() == "text")
				log("Error: " + reader.readElementText());
			//else
				//log("skipping error element: " + reader.name().toString());
		}
		reader.readNext();
	}

}

void JabberCtx::parseStreamStart() {
	sid = reader.attributes().value("id").toString();
	log("XML stream initialized.");
}

void JabberCtx::parseFeatures() {
	reader.readNext();
	while(!reader.atEnd() && reader.name() != "features") {
		if(reader.isStartElement()) {
			if(reader.name() == "mechanisms")
				parseMechanisms();
			else if(reader.name() == "session")
				sessionRequired = true;
			else if(reader.name() == "bind")
				bindResource();
			else if(reader.name() == "starttls") {
				tlsAvailable = true;
				if(!reader.isEndElement()) reader.readNext();
				tlsRequired = (reader.name() == "required");
				if(tlsRequired) log("tls required");
			} else
				log("skipping feature element: " + reader.name().toString());
		}
		reader.readNext();
	}
	if(sstate == SSSTARTTLS) {
		if(tlsAvailable) {
			if(tlsRequired) {
				sslSocket.startClientEncryption();
				log("Going encrypted.");
			} else
				start_tls();
		} else changeSessionState(SSAUTHORIZING);
	} else if(sstate == SSINITIALIZING) {
		changeSessionState(SSAUTHORIZING);
	}
}

void JabberCtx::parseMechanisms() {
	reader.readNext();
	while(!reader.atEnd() && reader.name() != "mechanisms") {
		if(reader.isStartElement() && reader.name() == "mechanism") {
			mechs << reader.readElementText();
		}
		reader.readNext();
	} 
}

void JabberCtx::parseChallenge() {
	QString ch = QString(QByteArray::fromBase64(reader.readElementText().toAscii()));
	log("Responding to challenge...");
	QByteArray response;

	QRegExp rre = QRegExp("realm=\\\"\\w*"), nre = QRegExp("nonce=\\\"\\w*");
	int rrei = rre.indexIn(ch), nrei = nre.indexIn(ch);
	if(nrei != -1) {
		QString realm = (rrei == -1 ? "" : ch.mid(rrei + 7, rre.matchedLength() - 7)),
			nonce = ch.mid(nrei + 7, nre.matchedLength() - 7);

		QCryptographicHash hash(QCryptographicHash::Md5);

		qsrand(QDateTime::currentDateTime().toTime_t());
		QString cnonce = QString("%1").arg(qrand());

		hash.reset();
		hash.addData((acc_info.username + ":" + realm + ":" + acc_info.password).toUtf8());
		QByteArray temp = hash.result();

		hash.reset();
		hash.addData(temp + (":" + nonce + ":" + cnonce).toUtf8());
		temp = hash.result();

		hash.reset();
		hash.addData(("AUTHENTICATE:xmpp/" + acc_info.host).toUtf8());
		QByteArray temp2 = hash.result();

		hash.reset();
		hash.addData((temp.toHex() + ":" + nonce + ":1:" + cnonce + ":auth:" + temp2.toHex()).toUtf8());

		QString res("username=\"" + acc_info.username + "\",realm=\"" + realm + "\",nonce=\"" + nonce + "\",cnonce=\"" + cnonce + "\",nc=1,qop=auth,digest-uri=\"xmpp/" +
			acc_info.host + "\",charset=utf-8,response=" + hash.result().toHex());

		response = res.toUtf8().toBase64();
	}

	writer.writeStartElement("response");
	writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-sasl");
	writer.writeCharacters(QString(response));
	writer.writeEndElement();

	sendWriteBuffer();
}

void JabberCtx::parseFailure() {
	reader.readNext();
	while(!reader.atEnd() && reader.name() != "failure") {
		if(reader.isStartElement()) {
		}
		reader.readNext();
	}
	endStream();
}

void JabberCtx::parseSuccess() {
	changeSessionState(SSLOGIN);
}

void JabberCtx::parseProceed() {
	if(sstate == SSSTARTTLS) {
		sslSocket.startClientEncryption();
		log("Going encrypted.");
	}
}

void JabberCtx::parseIq() {
	QString id = reader.attributes().value("id").toString(), 
		from = reader.attributes().value("from").toString();
	if(reader.attributes().value("type") == "result" && reader.attributes().value("id") == "bind") {
		reader.readNext();
		if(reader.name() == "bind") {
			reader.readNext();
			if(reader.name() == "jid") {
				jid = reader.readElementText();
				if(sessionRequired)
					startSession();
				else
					getRoster();
			}
		}
	} else if(reader.attributes().value("type") == "result" && reader.attributes().value("id") == "session") {
		getGroupDelimiter();
	} else if(reader.attributes().value("id") == "group_delimiter_get") {
		if(reader.attributes().value("type") == "result") {
			reader.readNext();
			if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:private") {
				reader.readNext();
				parseGroupDelimiter();
			}
		}
		getRoster();
	} else if(reader.attributes().value("type") == "result" && reader.attributes().value("id") == "roster_get") {
		reader.readNext();
		parseRosterQuery();
	} else if(reader.attributes().value("type") == "result" && reader.attributes().value("id") == "roster_remove") {
	} else if(reader.attributes().value("type") == "result" && reader.attributes().value("id") == "roster_update") {
	} else if(reader.attributes().value("type") == "result" && reader.attributes().value("id") == "roster_add") {
		sendPresence(from);
	} else if(reader.attributes().value("type") == "set") {
		reader.readNext();
		if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:roster") {
			reader.readNext();
			parseRosterItem();
			sendEmptyResult(id, from);
		} else
			sendIqError(id, from);
	} else if(reader.attributes().value("type") == "get") {
		reader.readNext();
		if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:version") {
			sendVersionInfoResult(id, from);
		} else if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "http://jabber.org/protocol/disco#info") {
			sendDiscoInfoResult(id, from);
		} else {
			sendIqError(id, from);
		}
	} else if(reader.attributes().value("type") != "error")
		sendIqError(id, from);

	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "iq")) reader.readNext();
}

void JabberCtx::start_tls() {
	writer.writeEmptyElement("starttls");
	writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-tls");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::authenticate() {
	if(mechs.contains("DIGEST-MD5")) {
		log("Starting authentication (DIGEST-MD5)...");
		writer.writeStartElement("auth");
		writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-sasl");
		writer.writeAttribute("mechanism", "DIGEST-MD5");
		writer.writeEndElement();
		sendWriteBuffer();
	} else if(mechs.contains("PLAIN")) {
		log("Authenticating (PLAIN)...");
		writer.writeStartElement("auth");
		writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-sasl");
		writer.writeAttribute("mechanism", "PLAIN");

		QByteArray resp;
		resp.append(acc_info.username + "@" + acc_info.host);
		resp.append('\0');
		resp.append(acc_info.username);
		resp.append('\0');
		resp.append(acc_info.password);
		writer.writeCharacters(resp.toBase64());
		writer.writeEndElement();
		sendWriteBuffer();
	} else {
		log("No compatible auth available - disconnecting");
		changeSessionState(SSTERMINATING);
	}
}

void JabberCtx::bindResource() {
	//<iq type='set' id='bind_2'>
	//	<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>
	//		<resource>someresource</resource>
	//	</bind>
	//</iq>
	log("Setting resource...");
	writer.writeStartElement("iq");
	writer.writeAttribute("type", "set");
	writer.writeAttribute("id", "bind");
		writer.writeStartElement("bind");
		writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-bind");
			writer.writeTextElement("resource", QHostInfo::localHostName());
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
}

void JabberCtx::startSession() {
	log("Starting session...");
	writer.writeStartElement("iq");
	writer.writeAttribute("type", "set");
	writer.writeAttribute("id", "session");
		writer.writeEmptyElement("session");
		writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-session");
	writer.writeEndElement();
	sendWriteBuffer();
}

void JabberCtx::getGroupDelimiter() {
	log("Getting subgroup delimiter...");
	writer.writeStartElement("iq");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "get");
	writer.writeAttribute("id", "group_delimiter_get");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("jabber:iq:private");
			writer.writeEmptyElement("roster");
			writer.writeDefaultNamespace("roster:delimiter");
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
}

void JabberCtx::getRoster() {
	//<iq from='juliet@example.com/balcony' type='get' id='roster_1'>
	//	<query xmlns='jabber:iq:roster'/>
	//</iq>
	log("Getting roster...");
	writer.writeStartElement("iq");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "get");
	writer.writeAttribute("id", "roster_get");
		writer.writeEmptyElement("query");
		writer.writeDefaultNamespace("jabber:iq:roster");
	writer.writeEndElement();
	sendWriteBuffer();
}

void JabberCtx::sendPresence(const QString &to) {
	QString type, show;
	if(sstate == SSNONE) return;

	switch(currentStatus) {
		case ST_OFFLINE: type = "unavailable"; break;
		case ST_INVISIBLE: type = "invisible"; break;
		case ST_ONLINE: break;
		case ST_SHORTAWAY: show = "away"; break;
		case ST_LONGAWAY: show = "xa"; break;
		case ST_DND: show = "dnd"; break;
		case ST_FREETOCHAT: show = "chat"; break;
	}

	writer.writeStartElement("presence");
	if(!to.isEmpty()) writer.writeAttribute("to", to);
	if(!type.isEmpty()) writer.writeAttribute("type", type);
	if(!show.isEmpty()) {
		writer.writeStartElement("show");
		writer.writeCharacters(show);
		writer.writeEndElement();
	}
	writer.writeEndElement();
	sendWriteBuffer();
	log("Sent presence.");
}

void JabberCtx::parseGroupDelimiter() {
	if(reader.isStartElement() && reader.name() == "roster") {
		reader.readNext();
		QString groupDelim = reader.text().toString();
		RosterGroup::setDelimiter(groupDelim);
		clist_i->set_group_delimiter("Jabber", account_id, groupDelim);
		log("Delim is " + groupDelim);
	}
}

void JabberCtx::parseRosterQuery() {
	if(reader.isStartElement() && reader.name() == "query") {
		while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "query")) {
			reader.readNext();
			if(reader.isStartElement() && reader.name() == "item")
				parseRosterItem();
		}
	}
	log("Got roster.");
	changeSessionState(SSOK);
}

void JabberCtx::setDetails(RosterItem *item, const QString &group, const QString &name, SubscriptionType sub) {
	RosterGroup *current_group = item->getGroup(),
		*new_group = roster.get_group(group);
	if(current_group != new_group) {
		if(current_group) current_group->removeChild(item);
		if(new_group) new_group->addChild(item);
		clist_i->set_group("Jabber", account_id, item->getJID(), group);
	}
	item->setName(name);
	clist_i->set_label("Jabber", account_id, item->getJID(), name);
	item->setSubscription(sub);
}

void JabberCtx::addItem(const QString &jid, const QString &name, const QString &group, SubscriptionType sub) {
	RosterGroup *gr = roster.get_group(group);
	if(!gr) {
		QStringList subgroups = group.split(RosterGroup::getDelimiter());
		if(subgroups.size()) {
			gr = &roster;
			RosterGroup *parent = 0;
			while(gr) {
				parent = gr;
				gr = static_cast<RosterGroup *>(gr->child(subgroups.at(0)));
				if(gr) {
					subgroups.removeAt(0);
					qDebug() << "found group:" << gr->getLabel();
				}
			}
			QModelIndex i;
			while(subgroups.size()) {
				gr = parent->get_group(subgroups.at(0), true);
				qDebug() << "created group:" << gr->getLabel();
				subgroups.removeAt(0);
				parent = gr;
			}
		} else 
			gr = &roster;
	}
	RosterItem *item = new RosterItem(jid, name, sub, gr);
	gr->addChild(item);

	clist_i->add_contact("Jabber", account_id, jid, name, ST_OFFLINE, group);
}


void JabberCtx::parseRosterItem() {
	QString jid = reader.attributes().value("jid").toString(),
		name = reader.attributes().value("name").toString(),
		subscription = 	reader.attributes().value("subscription").toString(),
		group;

	reader.readNext();
	if(reader.isStartElement() && reader.name() == "group") {
		group = reader.readElementText();
	}

	// ensure non-empty name
	if(name.isEmpty()) name = jid.split("@").at(0);

	RosterItem *item = roster.get_item(jid);
	if(subscription == "remove" && item) {
		clist_i->remove_contact("Jabber", account_id, jid);
		item->getGroup()->removeChild(item);
		delete item;
	} else if(item) {
		setDetails(item, group, name, RosterItem::string2sub(subscription));
	} else {
		addItem(jid, name, group, RosterItem::string2sub(subscription));
	}
}

GlobalStatus presenceToStatus(PresenceType pt) {
	switch(pt) {
		case PT_UNAVAILABLE:	return ST_OFFLINE;
		case PT_INVISIBLE:		return ST_INVISIBLE;
		case PT_ONLINE:			return ST_ONLINE;
		case PT_AWAY:			return ST_SHORTAWAY;
		case PT_DND:			return ST_DND;
		case PT_XA:				return ST_LONGAWAY;
		case PT_CHAT:			return ST_FREETOCHAT;
	}
	return ST_OFFLINE;
}

bool JabberCtx::setPresence(const QString &full_jid, PresenceType presence, const QString &msg) {
	Resource *r = roster.get_resource(full_jid, false);
	if(!r) {
		RosterItem *item = roster.get_item(Roster::full_jid2jid(full_jid));
		if(!item) return false;

		r = roster.get_resource(full_jid, true);
		r->setPresence(presence);
		r->setPresenceMessage(msg);
	} else {
		r->setPresence(presence);
		r->setPresenceMessage(msg);
	}

	clist_i->set_status("Jabber", account_id, Roster::full_jid2jid(full_jid), presenceToStatus(presence));
	return true;
}

void JabberCtx::parsePresence() {
	QString jid = reader.attributes().value("from").toString(), 
		presence = "available",
		name = jid,
		msg;

	if(!reader.attributes().value("type").isNull()) presence = reader.attributes().value("type").toString();

	if(presence == "subscribe") {
		if(QMessageBox::information(0, "Subscription Request", "Approve subscription from " + jid + "?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
			sendGrant(jid);			
		} else {
			sendRevoke(jid);
		}
	} else if(presence == "subscribed") {
	} else if(presence == "unsubscribe") {
	} else if(presence == "unsubscribed") {
	} else {
		while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "presence")) {
			reader.readNext();
			if(!reader.atEnd() && reader.isStartElement() && reader.name() == "show")
				presence = reader.readElementText();
			if(!reader.atEnd() && reader.isStartElement() && reader.name() == "status") {
				msg = reader.readElementText();
			}
		}

		setPresence(jid, Resource::string2pres(presence), msg);
	}
}

void JabberCtx::msgSend(const QString &cid, const QString &msg, int id) {
	writer.writeStartElement("message");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("to", cid);
	writer.writeAttribute("type", "chat");
	writer.writeAttribute("id", QString("%1").arg(id));
	writer.writeAttribute("xml:lang", "en");
		writer.writeTextElement("body", msg);
	writer.writeEndElement();
	sendWriteBuffer();
	emit msgAck(id);
	log("Sent message to " + id);
}

void JabberCtx::parseMessage() {
	QString source = reader.attributes().value("from").toString(), body;
	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "message")) {
		reader.readNext();
		if(!reader.atEnd() && reader.isStartElement() && reader.name() == "body") {
			body = reader.readElementText();
			log("Received message from " + source);
		}
	}

	Resource *r = roster.get_resource(source,  false);
	if(r) {
		RosterItem *i = r->getItem();
		QString id = i->getJID();

		emit msgRecv(account_id, id, body);
	} else {
		log("message from unknown resource ignored: " + source);
	}
}

void JabberCtx::sendIqError(const QString &id, const QString &sender, const QString &errorType, const QString &definedCondition) {
	log("Sending iq error to " + sender);
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	writer.writeAttribute("to", sender);
	writer.writeAttribute("type", "error");
		writer.writeStartElement("error");
		writer.writeAttribute("type", errorType);
			writer.writeEmptyElement(definedCondition);
			writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-stanzas");
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
}

void JabberCtx::sendEmptyResult(const QString &id, const QString &sender) {
	log("Sending empty result to " + sender);
	writer.writeEmptyElement("iq");
	writer.writeAttribute("id", id);
	//writer.writeAttribute("to", sender);
	writer.writeAttribute("type", "result");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::sendVersionInfoResult(const QString &id, const QString &sender) {
	log("Sending version info to " + sender);
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	writer.writeAttribute("to", sender);
	//writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "result");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("jabber:iq:version");
			writer.writeTextElement("name", "sjc - sje's Simple Jabber Client");
			writer.writeTextElement("version", "0.0.1a");
		writer.writeEndElement(); // query
	writer.writeEndElement(); // iq
	sendWriteBuffer();
	//log("sent version info to " + sender);
}

void JabberCtx::sendDiscoInfoResult(const QString &id, const QString &sender) {
	log("Sending disco info to " + sender);
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	writer.writeAttribute("to", sender);
	//writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "result");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("http://jabber.org/protocol/disco#info");
			// identities
			//writer.writeEmptyElement("entity");
			//writer.writeAttribute("category", "");
			//writer.writeAttribute("type", "");
			writer.writeEmptyElement("entity");
			writer.writeAttribute("category", "client");
			writer.writeAttribute("type", "pc");
			// features
			//writer.writeEmptyElement("feature");
			//writer.writeAttribute("var", "");
			writer.writeEmptyElement("feature");
			writer.writeAttribute("var", "http://jabber.org/protocol/disco#info");
			writer.writeEmptyElement("feature");
			writer.writeAttribute("var", "jabber:iq:version");
		writer.writeEndElement(); // query
	writer.writeEndElement(); // iq
	sendWriteBuffer();
	//log("sent version info to " + sender);
}

void JabberCtx::addContact(const QString &jid) {
	RosterItem *item = roster.get_item(mid);
	if(!item) {

		//<iq from='juliet@example.com/balcony' type='set' id='roster_2'>
		//	<query xmlns='jabber:iq:roster'>
		//		<item jid='nurse@example.com' name='Nurse'>
		//			<group>Servants</group>
		//		</item>
		//	</query>
		//</iq>
		writer.writeStartElement("iq");
		writer.writeAttribute("type", "set");
		writer.writeAttribute("id", "roster_add");
			writer.writeStartElement("query");
			writer.writeDefaultNamespace("jabber:iq:roster");
				writer.writeStartElement("item");
				writer.writeAttribute("jid", jid);
				//writer.writeAttribute("name", d.getName());
				//if(!d.getGroup().isEmpty()) {
				//	writer.writeTextElement("group", d.getGroup());
				//}
				writer.writeEndElement();
			writer.writeEndElement();
		writer.writeEndElement();
		sendWriteBuffer();
	} else
		qWarning() << "JID" << jid << "already exists for account" << account_id;
}

bool JabberCtx::directSend(const QString &text) {
	if(sstate == SSOK) {
		sendBuffer.write(text.toUtf8());
		sendWriteBuffer();
		return true;
	} else {
		log("direct send failed", LMT_ERROR);
	}
	return false;
}

void JabberCtx::addRosterItem() {
	//RosterTreeNode *n = rosterModel.node(ui.rosterTreeView->currentIndex());
	//if(n->type() == RTNT_RESOURCE) n = n->getParent();
	//if(n->type() == RTNT_ITEM) n = n->getParent();
	//NewRosterItemDialog d((n->type() == RTNT_GROUP ? n->getName() : ""));
	//if(d.exec() == QDialog::Accepted) {
	//	//<iq from='juliet@example.com/balcony' type='set' id='roster_2'>
	//	//	<query xmlns='jabber:iq:roster'>
	//	//		<item jid='nurse@example.com' name='Nurse'>
	//	//			<group>Servants</group>
	//	//		</item>
	//	//	</query>
	//	//</iq>
	//	writer.writeStartElement("iq");
	//	writer.writeAttribute("type", "set");
	//	writer.writeAttribute("id", "roster_add");
	//		writer.writeStartElement("query");
	//		writer.writeDefaultNamespace("jabber:iq:roster");
	//			writer.writeStartElement("item");
	//			writer.writeAttribute("jid", d.getJID());
	//			writer.writeAttribute("name", d.getName());
	//			if(!d.getGroup().isEmpty()) {
	//				writer.writeTextElement("group", d.getGroup());
	//			}
	//			writer.writeEndElement();
	//		writer.writeEndElement();
	//	writer.writeEndElement();
	//	sendWriteBuffer();
	//}
}

void JabberCtx::editRosterItem() {
	RosterItem *item = roster.get_item(mid);
	if(item) {
		RosterGroup *g = item->getGroup();
		NewRosterItemDialog d(item->getJID(), item->getName(), g->getFullName());
		if(d.exec() == QDialog::Accepted) {
			//<iq from='juliet@example.com/balcony' type='set' id='roster_2'>
			//	<query xmlns='jabber:iq:roster'>
			//		<item jid='nurse@example.com' name='Nurse'>
			//			<group>Servants</group>
			//		</item>
			//	</query>
			//</iq>
			writer.writeStartElement("iq");
			writer.writeAttribute("type", "set");
			writer.writeAttribute("id", "roster_update");
				writer.writeStartElement("query");
				writer.writeDefaultNamespace("jabber:iq:roster");
					writer.writeStartElement("item");
					writer.writeAttribute("jid", d.getJID());
					writer.writeAttribute("name", d.getName());
					if(!d.getGroup().isEmpty()) {
						writer.writeTextElement("group", d.getGroup());
					}
					writer.writeEndElement();
				writer.writeEndElement();
			writer.writeEndElement();
			sendWriteBuffer();
		}
	}
}

void JabberCtx::removeRosterItem() {
	RosterItem *item = roster.get_item(mid);
	if(item) {
		QString jid = item->getJID();
		//<iq from='juliet@example.com/balcony' type='set' id='delete_1'>
		//	<query xmlns='jabber:iq:roster'>
		//		<item jid='nurse@example.com' subscription='remove'/>
		//	</query>
		//</iq>
		writer.writeStartElement("iq");
		writer.writeAttribute("type", "set");
		writer.writeAttribute("id", "roster_remove");
			writer.writeStartElement("query");
			writer.writeDefaultNamespace("jabber:iq:roster");
				writer.writeEmptyElement("item");
				writer.writeAttribute("jid", jid);
				writer.writeAttribute("subscription", "remove");
			writer.writeEndElement();
		writer.writeEndElement();
		sendWriteBuffer();
	}
}

void JabberCtx::sendGrant(const QString &jid) {
	log("Granting subscription to " + jid);
	writer.writeEmptyElement("presence");
	writer.writeAttribute("to", jid);
	writer.writeAttribute("type", "subscribed");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::grantSubscription() {
	sendGrant(mid);
}

void JabberCtx::sendRevoke(const QString &jid) {
	log("Revoking subscription from " + jid);
	//<presence to='romeo@example.net' type='unsubscribed'/>
	writer.writeEmptyElement("presence");
	writer.writeAttribute("to", jid);
	writer.writeAttribute("type", "unsubscribed");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::revokeSubscription() {
	sendRevoke(mid);
}

void JabberCtx::requestSubscription() {
	log("Requesting subscription from " + mid);
	writer.writeEmptyElement("presence");
	writer.writeAttribute("to", jid);
	writer.writeAttribute("type", "subscribe");
	writer.writeCharacters("");
	sendWriteBuffer();
}


void JabberCtx::setAllOffline() {
	QStringList all_ids = roster.all_items();
	foreach(QString id, all_ids) {
		clist_i->set_status("Jabber", account_id, id, ST_OFFLINE);
	}
}
