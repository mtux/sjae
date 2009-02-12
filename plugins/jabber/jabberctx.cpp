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
#include <QStringList>

#include "newrosteritemdialog.h"
#include "gatewayregister.h"

#include <clist_i.h>

JabberCtx::JabberCtx(Account *acc, CoreI *core, QObject *parent)
	: QObject(parent), account(acc), useSSL(false), core_i(core), sstate(SSNONE), writer(&sendBuffer), 
		sessionRequired(false), tlsAvailable(false), tlsRequired(false),
		priority(DEFAULT_PRIORITY)
{
	sendBuffer.open(QIODevice::WriteOnly);

	connect(&sslSocket, SIGNAL(readyRead()), this, SLOT(readSocket()), Qt::QueuedConnection);
	connect(&sslSocket, SIGNAL(connected()), this, SLOT(socketConnected()));
	connect(&sslSocket, SIGNAL(encrypted()), this, SLOT(socketEncrypted()));
	connect(&sslSocket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
	connect(&sslSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
	connect(&sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors(const QList<QSslError> &)));
	connect(&sslSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));

	sslSocket.setProtocol(QSsl::AnyProtocol);

	clist_i = (CListI *)core_i->get_interface(INAME_CLIST);
	if(clist_i) {

		editRosterItemAction = clist_i->add_contact_action(account, "Edit...");
		connect(editRosterItemAction, SIGNAL(triggered()), this, SLOT(editRosterItem()));

		removeRosterItemAction = clist_i->add_contact_action(account, "Remove");
		connect(removeRosterItemAction, SIGNAL(triggered()), this, SLOT(removeRosterItem()));


		grantAction = clist_i->add_contact_action(account, "Grant");
		connect(grantAction, SIGNAL(triggered()), this, SLOT(grantSubscription()));

		revokeAction = clist_i->add_contact_action(account, "Revoke");
		connect(revokeAction, SIGNAL(triggered()), this, SLOT(revokeSubscription()));

		requestAction = clist_i->add_contact_action(account, "Request");
		connect(requestAction, SIGNAL(triggered()), this, SLOT(requestSubscription()));

		//	void aboutToShowMenu(const QString &proto_name, const QString &account_id, const QString &id);

		//connect(clist_i, SIGNAL(aboutToShowContactMenu(Contact &)), this, SLOT(aboutToShowContactMenu(const QString &, const QString &, const QString &)));
		//connect(clist_i, SIGNAL(aboutToShowGroupMenu(const QString &, const QString &, const QString &)), this, SLOT(aboutToShowGroupMenu(const QString &, const QString &, const QString &)));
	}

	events_i = (EventsI *)core_i->get_interface(INAME_EVENTS);
	events_i->add_event_listener(this, UUID_SHOW_CONTACT_MENU);
	events_i->add_event_listener(this, UUID_SHOW_GROUP_MENU);

	keepAliveTimer.setInterval(30000);
	connect(&keepAliveTimer, SIGNAL(timeout()), this, SLOT(sendKeepAlive()));
}

void JabberCtx::setAccountInfo(Account *acc) {
	account = acc;
}

JabberCtx::~JabberCtx()
{
	requestStatus(ST_OFFLINE);
	if(sstate != SSNONE) changeSessionState(SSNONE);
	disconnect(&sslSocket, SIGNAL(readyRead()), this, SLOT(readSocket()));
	disconnect(&sslSocket, SIGNAL(connected()), this, SLOT(socketConnected()));
	disconnect(&sslSocket, SIGNAL(encrypted()), this, SLOT(socketEncrypted()));
	disconnect(&sslSocket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
	disconnect(&sslSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
	disconnect(&sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors(const QList<QSslError> &)));
	disconnect(&sslSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));

	events_i->remove_event_listener(this, UUID_SHOW_CONTACT_MENU);
	events_i->remove_event_listener(this, UUID_SHOW_GROUP_MENU);
}

Account *JabberCtx::get_account_info() {
	return account;
}

bool JabberCtx::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_SHOW_CONTACT_MENU) {
		ShowContactMenu &sm = static_cast<ShowContactMenu &>(e);

		bool vis = (sm.contact->account == account);
		//newRosterItemAction->setVisible(vis);

		removeRosterItemAction->setVisible(vis);
		editRosterItemAction->setVisible(vis);

		if(vis) {
			RosterItem *item = roster.get_item(sm.contact->contact_id);
			SubscriptionType sub = item->getSubscription();

			bool to = (sub == ST_BOTH || sub == ST_TO),
				from = (sub == ST_BOTH || sub == ST_FROM);

			grantAction->setVisible(vis && !to);
			revokeAction->setVisible(vis && to);

			requestAction->setVisible(vis && !from);

			mid = sm.contact->contact_id;
		} else {
			grantAction->setVisible(false);
			revokeAction->setVisible(false);

			requestAction->setVisible(false);
		}
	} else if(e.uuid == UUID_SHOW_GROUP_MENU) {
		removeRosterItemAction->setVisible(false);
		editRosterItemAction->setVisible(false);
		grantAction->setVisible(false);
		revokeAction->setVisible(false);
		requestAction->setVisible(false);
	}
	return true;
}

void JabberCtx::showMessage(const QString &message) {
	qDebug() << ("Jabber message (" + account->account_id + "):") << message;
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
	account->desiredStatus = gs;

	if(account->desiredStatus == account->status) return;

	if(sstate == SSOK) {
		if(account->desiredStatus == ST_OFFLINE)
			connectToServer(false);
		else
			setStatus(gs);
	} else if(sstate == SSNONE && gs != ST_OFFLINE) {
		connectToServer(true);
	} else if(sstate != SSOK && gs == ST_OFFLINE) {
		changeSessionState(SSNONE);
	} else {
		qWarning() << "Jabber account" << account->account_id << "status change request while in invalid state - ignored";
	}
}

void JabberCtx::setStatus(GlobalStatus gs) {
	account->status = gs;
	AccountChanged ac(account, account->proto);
	events_i->fire_event(ac);
	if(sstate == SSOK)
		sendPresence();
}

void JabberCtx::changeSessionState(const SessionState &newState) {
	sstate = newState;
	switch(newState) {
		case SSNONE:
			keepAliveTimer.stop();
			setStatus(ST_OFFLINE);
			showMessage("Disconnected");
			sslSocket.close();
			//newRosterItemAction->setEnabled(false);
			//setAllOffline();
			clist_i->remove_all_contacts(account);
			roster.clear();
			break;
		case SSSTARTSSL:
			setStatus(ST_CONNECTING);
			showMessage("Connecting (encrypted)...");
			sslSocket.connectToHostEncrypted(connectionHost.isEmpty() ? account->host : connectionHost, account->port);
			break;
		case SSSTARTTLS:
			setStatus(ST_CONNECTING);
			showMessage("Connecting...");
			sslSocket.connectToHost(connectionHost.isEmpty() ? account->host : connectionHost, account->port);
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
			setStatus(account->desiredStatus);
			showMessage("Ok");
			//newRosterItemAction->setEnabled(true);
			keepAliveTimer.start();
			break;
		case SSTERMINATING:
			keepAliveTimer.stop();
			setStatus(ST_OFFLINE);
			showMessage("Disconnecting...");
			endStream();
			break;
	}
}

void JabberCtx::sendKeepAlive() {
	if(sstate == SSOK)
		sslSocket.write(" ", 1);
}

void JabberCtx::sendWriteBuffer() {
	keepAliveTimer.stop();
	keepAliveTimer.start();

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
	
}

void JabberCtx::blockingReadSocketMore() {
	sslSocket.waitForReadyRead();

	if(sslSocket.bytesAvailable() > 0) {
		QByteArray data = sslSocket.read(sslSocket.bytesAvailable());
		log(QString().append(data), LMT_RECV);

		reader.addData(data);
	}
}

void JabberCtx::readMoreIfNecessary() {
	reader.readNext();
	if(reader.atEnd() && reader.hasError() && reader.error() == QXmlStreamReader::PrematureEndOfDocumentError) {
		blockingReadSocketMore();
		reader.readNext();
	}
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
		log("Connecting to " + (connectionHost.isEmpty() ? account->host : connectionHost) + "...");
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
	writer.writeAttribute("to", account->host);
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
	do {
		readMoreIfNecessary();
		if(reader.isStartElement()) {
			if(reader.name() == "text")
				log("Error: " + reader.readElementText());
			//else
				//log("skipping error element: " + reader.name().toString());
		}
	} while(reader.name() != "error");
}

void JabberCtx::parseStreamStart() {
	sid = reader.attributes().value("id").toString();
	log("XML stream initialized.");
}

void JabberCtx::parseFeatures() {
	do {
		readMoreIfNecessary();
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
	} while(!reader.atEnd() && reader.name() != "features");
	
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
	do {
		readMoreIfNecessary();
		if(reader.isStartElement() && reader.name() == "mechanism") {
			mechs << reader.readElementText();
		}
	} while(reader.name() != "mechanisms");
}

void JabberCtx::parseChallenge() {
	QString ch = QString(QByteArray::fromBase64(reader.readElementText().toAscii()));
	//log("Responding to challenge: " + ch);
	QByteArray response;

	QRegExp rre = QRegExp("realm=\\\"[^\\\"]*"), nre = QRegExp("nonce=\\\"[^\\\"]*");
	int rrei = rre.indexIn(ch), nrei = nre.indexIn(ch);
	if(nrei != -1) {
		QString realm = (rrei == -1 ? account->host : ch.mid(rrei + 7, rre.matchedLength() - 7)),
			nonce = ch.mid(nrei + 7, nre.matchedLength() - 7);

		qsrand(QDateTime::currentDateTime().toTime_t());
		QCryptographicHash hash(QCryptographicHash::Md5);

		hash.reset();
		hash.addData(QString("%1").arg(qrand()).toUtf8());

		QString cnonce = QString(hash.result().toHex()).rightJustified(32, '0');

		hash.reset();
		hash.addData((account->username + ":" + realm + ":" + account->password).toUtf8());
		QByteArray temp = hash.result();

		hash.reset();
		hash.addData(temp + (":" + nonce + ":" + cnonce).toUtf8());
		temp = hash.result();

		hash.reset();
		hash.addData(("AUTHENTICATE:xmpp/" + account->host).toUtf8());
		QByteArray temp2 = hash.result();

		hash.reset();
		hash.addData((temp.toHex() + ":" + nonce + ":00000001:" + cnonce + ":auth:" + temp2.toHex()).toUtf8());

		QString res = QString("username=\"%1\",realm=\"%2\",nonce=\"%3\",cnonce=\"%4\",nc=%5,qop=auth,digest-uri=\"xmpp/%6\",charset=utf-8,response=%7")
			.arg(account->username)
			.arg(realm)
			.arg(nonce)
			.arg(cnonce)
			.arg("00000001")
			.arg(account->host)
			.arg(QString(hash.result().toHex()).rightJustified(32, '0'));


		response = res.toUtf8().toBase64();
	}

	writer.writeStartElement("response");
	writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-sasl");
	writer.writeCharacters(QString(response));
	writer.writeEndElement();

	sendWriteBuffer();
}

void JabberCtx::parseFailure() {
	do {
		readMoreIfNecessary();
		if(reader.isStartElement()) {
		}
	} while(reader.name() != "failure");
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
	if(reader.attributes().value("type") == "result") {
		if(reader.attributes().value("id") == "bind") {
			readMoreIfNecessary();
			if(reader.name() == "bind") {
				readMoreIfNecessary();
				if(reader.name() == "jid") {
					jid = reader.readElementText();
					if(sessionRequired)
						startSession();
					else
						getRoster();
				}
			}
		} else if(reader.attributes().value("id") == "session") {
			sendIqQueryDiscoInfo(account->host);
			getGroupDelimiter();
		} else if(reader.attributes().value("id") == "group_delimiter_get") {
			readMoreIfNecessary();
			if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:private") {
				readMoreIfNecessary();
				parseGroupDelimiter();
			}
			getRoster();
		} else if(reader.attributes().value("id") == "roster_get") {
			readMoreIfNecessary();
			parseRosterQuery();
		} else if(reader.attributes().value("id") == "roster_remove") {
		} else if(reader.attributes().value("id") == "roster_update") {
		} else if(reader.attributes().value("id") == "roster_add") {
			sendPresence(from);
		} else if(reader.attributes().value("id") == "gateway_register2") {
			//addContact(from);
			sendRequestSubscription(from);
			//sendPresence(from);
			//sendGrant(from);
		} else if(reader.attributes().value("id") == "gateway_unregister") {
		} else {
			readMoreIfNecessary();
			if(reader.isStartElement() && reader.name() == "query") {
				if(reader.namespaceUri() == "http://jabber.org/protocol/disco#info")
					parseDiscoInfoResult(from);
				else if(reader.namespaceUri() == "http://jabber.org/protocol/disco#items")
					parseDiscoItemsResult(from);
				else if(reader.namespaceUri() == "jabber:iq:register")
					parseRegisterResult(from);
				else
					sendIqError(id, from);
			} else
				sendIqError(id, from);
		}
	} else if(reader.attributes().value("type") == "set") {
		readMoreIfNecessary();
		if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:roster") {
			readMoreIfNecessary();
			parseRosterItem();
			sendEmptyResult(id, from);
		} else
			sendIqError(id, from);
	} else if(reader.attributes().value("type") == "get") {
		readMoreIfNecessary();
		if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:version") {
			sendVersionInfoResult(id, from);
		} else if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "http://jabber.org/protocol/disco#info") {
			sendDiscoInfoResult(id, from);
		} else {
			sendIqError(id, from);
		}
	} else if(reader.attributes().value("type") == "error") {
		if(reader.attributes().value("id") == "group_delimiter_get") {
			getRoster();
		}
	} else
		sendIqError(id, from);

	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "iq")) readMoreIfNecessary();
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
		resp.append(account->username + "@" + account->host);
		resp.append('\0');
		resp.append(account->username);
		resp.append('\0');
		resp.append(account->password);
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

	switch(account->status) {
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
		if(type != "unavailable" && DEFAULT_PRIORITY != 0) {
			writer.writeStartElement("priority");
			writer.writeCharacters(QString("%1").arg(DEFAULT_PRIORITY));
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
		if(!groupDelim.isEmpty()) {
			RosterGroup::setDelimiter(groupDelim);
			clist_i->set_group_delimiter(account, groupDelim);
			log("Delim is " + groupDelim);
		}
	}
}

void JabberCtx::parseRosterQuery() {
	if(reader.isStartElement() && reader.name() == "query") {
		while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "query")) {
			readMoreIfNecessary();
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
	}
	item->setName(name);
	item->setSubscription(sub);

	ContactChanged cc(item->getContact(), this);
	events_i->fire_event(cc);
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
	RosterItem *item = new RosterItem(account, jid, name, sub, gr);
	gr->addChild(item);

	ContactChanged cc(item->getContact(), this);
	events_i->fire_event(cc);
}


void JabberCtx::parseRosterItem() {
	QString jid = reader.attributes().value("jid").toString(),
		name = reader.attributes().value("name").toString(),
		subscription = 	reader.attributes().value("subscription").toString(),
		ask = reader.attributes().value("ask").toString(),
		group;

	readMoreIfNecessary();
	if(reader.isStartElement() && reader.name() == "group") {
		group = reader.readElementText();
	}

	// ensure non-empty name
	if(name.isEmpty()) name = jid.split("@").at(0);

	RosterItem *item = roster.get_item(jid);
	if(subscription == "remove" && item) {
		ContactChanged cc(item->getContact(), this);
		cc.removed = true;
		events_i->fire_event(cc);

		item->getGroup()->removeChild(item);
		delete item;
	} else if(item) {
		setDetails(item, group, name, RosterItem::string2sub(subscription));
		if(ask == "subscribe") {
			//emit grantRequested(jid, account_id);
		}
	} else {
		log("Adding id to roster: " + jid + "(group: " + group + ")");
		addItem(jid, name, group, RosterItem::string2sub(subscription));
		if(ask == "subscribe") {
			//emit grantRequested(jid, account_id);
		}
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

GlobalStatus JabberCtx::getContactStatus(const QString &contact_id) {
	RosterItem *item = roster.get_item(contact_id);
	if(item) {
		Resource *r = item->get_active_resource();
		return presenceToStatus(r->getPresence());
	}
	return ST_OFFLINE;
}

void JabberCtx::setConnectionHost(const QString &host) {
	connectionHost = host;
}

bool JabberCtx::setPresence(const QString &full_jid, PresenceType presence, const QString &msg, int prio) {
	//log("setting presence for resource: " + full_jid);
	RosterItem *item = roster.get_item(Roster::full_jid2jid(full_jid));
	if(!item) return false;

	Resource *r = roster.get_resource(full_jid, false);
	if(!r) r = roster.get_resource(full_jid, true);

	r->setPresence(presence);
	r->setPresenceMessage(msg);
	r->updateLastActivity();
	r->setPriority(prio);

	// application contact status is based on 'active' resource
	r = item->get_active_resource();
	item->getContact()->status = presenceToStatus(r->getPresence());
	if(r->getPresenceMessage().isEmpty())
		item->getContact()->properties.remove("status_msg");
	else
		item->getContact()->properties["status_msg"] = r->getPresenceMessage();

	ContactChanged cc(item->getContact(), this);
	events_i->fire_event(cc);

	return true;
}

void JabberCtx::parsePresence() {
	QString jid = reader.attributes().value("from").toString(), 
		presence = "available",
		presenceType,
		name = jid,
		nick,
		msg;
	int prio = 0;

	presenceType = reader.attributes().value("type").toString();

	if(presenceType == "subscribe") {
		emit grantRequested(jid, account->account_id);
	} else if(presenceType == "subscribed") {
		//sendPresence(jid);
	} else if(presenceType == "unsubscribe") {
	} else if(presenceType == "unsubscribed") {
	} else if(presenceType == "unavailable") {
		presence = "unavailable";
	}

	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "presence")) {
		readMoreIfNecessary();
		if(!reader.atEnd() && reader.isStartElement() && reader.name() == "show") {
			presence = reader.readElementText();
		} else if(!reader.atEnd() && reader.isStartElement() && reader.name() == "status") {
			msg = reader.readElementText();
		} else if(!reader.atEnd() && reader.isStartElement() && reader.name() == "nick") {
			nick = reader.readElementText();
		} else if(!reader.atEnd() && reader.isStartElement() && reader.name() == "priority") {
			prio = reader.readElementText().toInt();
		}
	}

	if(presenceType.isEmpty() || presenceType == "unavailable") 
		setPresence(jid, Resource::string2pres(presence), msg, prio);

	/*
	if(!nick.isEmpty()) {
		writer.writeStartElement("iq");
		writer.writeAttribute("type", "set");
		writer.writeAttribute("id", "roster_update");
			writer.writeStartElement("query");
			writer.writeDefaultNamespace("jabber:iq:roster");
				writer.writeStartElement("item");
				writer.writeAttribute("jid", jid);
				writer.writeAttribute("name", nick);
				writer.writeEndElement();
			writer.writeEndElement();
		writer.writeEndElement();
		sendWriteBuffer();
	}
	*/
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
	log("Sent message to " + cid);
}

void JabberCtx::parseMessageBody(const QString &source) {
	QString body = reader.readElementText();
	log("Received message from " + source);

	Resource *r = roster.get_resource(source,  false);
	if(r) {
		r->updateLastActivity();

		RosterItem *i = r->getItem();
		QString id = i->getJID();

		MessageRecv mr(body, 0, i->getContact(), this);
		events_i->fire_event(mr);
	} else {
		log("message from unknown resource ignored: " + source);
	}
}

void JabberCtx::parseMessage() {
	QString source = reader.attributes().value("from").toString(), body;
	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "message")) {
		readMoreIfNecessary();
		if(!reader.atEnd() && reader.isStartElement() && reader.name() == "body")
			parseMessageBody(source);
	}
}

void JabberCtx::sendIqQueryDiscoInfo(const QString &entity_jid, const QString &node) {
	writer.writeStartElement("iq");
	writer.writeAttribute("type", "get");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("to", entity_jid);
	writer.writeAttribute("id", "get_disco_info");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("http://jabber.org/protocol/disco#info");
		if(!node.isEmpty())
			writer.writeAttribute("node", node);
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
}

void JabberCtx::sendIqQueryDiscoItems(const QString &entity_jid, const QString &node) {
	writer.writeStartElement("iq");
	writer.writeAttribute("type", "get");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("to", entity_jid);
	writer.writeAttribute("id", "get_disco_items");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("http://jabber.org/protocol/disco#items");
		if(!node.isEmpty())
			writer.writeAttribute("node", node);
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
}

void JabberCtx::parseDiscoInfoResult(const QString &entity) {
	DiscoInfo discoInfo;
	discoInfo.account_id = account->account_id;
	discoInfo.entity = entity;
	discoInfo.node = reader.attributes().value("node").toString();
	
	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "query")) {
		readMoreIfNecessary();
		if(!reader.atEnd() && reader.isStartElement() && reader.name() == "identity") {
			Identity ident;
			ident.category = reader.attributes().value("category").toString();
			ident.type = reader.attributes().value("type").toString();
			ident.name = reader.attributes().value("name").toString(); // optional

			discoInfo.indentities.append(ident);

			if(ident.category == "gateway") {
				emit gotGateway(account->account_id, entity);
			}

		} else if(!reader.atEnd() && reader.isStartElement() && reader.name() == "feature") {
			Feature feature;
			feature.var = reader.attributes().value("var").toString();
			if(feature.var == "http://jabber.org/protocol/disco#items")
				sendIqQueryDiscoItems(discoInfo.entity, discoInfo.node);
			
			discoInfo.features.append(feature);
		}
	}

	log("Parsed disco info for entity " + discoInfo.entity, LMT_NORMAL);
	emit gotDiscoInfo(discoInfo);
}

void JabberCtx::parseDiscoItemsResult(const QString &entity) {
	DiscoItems discoItems;
	discoItems.account_id = account->account_id;
	discoItems.entity = entity;
	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "query")) {
		readMoreIfNecessary();
		if(!reader.atEnd() && reader.isStartElement() && reader.name() == "item") {
			Item item;
			item.jid = reader.attributes().value("jid").toString();
			item.name = reader.attributes().value("name").toString();
			item.node = reader.attributes().value("node").toString();

			discoItems.items.append(item);

			// get info about root host items - necessary to find gateways
			if(entity == account->host)
				sendIqQueryDiscoInfo(item.jid, item.node);
		}
	}
	log("Parsed disco items for entity " + discoItems.entity, LMT_NORMAL);
	emit gotDiscoItems(discoItems);
}

void JabberCtx::sendIqError(const QString &id, const QString &sender, const QString &errorType, const QString &definedCondition) {
	//log("Sending iq error to: " + sender);
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	if(!sender.isEmpty())
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
	//log("Sending empty result to: " + sender);
	writer.writeEmptyElement("iq");
	writer.writeAttribute("id", id);
	if(!sender.isEmpty())
		writer.writeAttribute("to", sender);
	writer.writeAttribute("type", "result");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::sendVersionInfoResult(const QString &id, const QString &sender) {
	//log("Sending version info to " + sender);
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	if(!sender.isEmpty())
		writer.writeAttribute("to", sender);
	//writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "result");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("jabber:iq:version");
			writer.writeTextElement("name", "saje (saje.googlecode.com)");
			writer.writeTextElement("version", core_i->version());
		writer.writeEndElement(); // query
	writer.writeEndElement(); // iq
	sendWriteBuffer();
	//log("sent version info to " + sender);
}

void JabberCtx::sendDiscoInfoResult(const QString &id, const QString &sender) {
	//log("Sending disco info to " + sender);
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	if(!sender.isEmpty())
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
		qWarning() << "JID" << jid << "already exists for account" << account->account_id;

	sendRequestSubscription(jid);
}

bool JabberCtx::directSend(const QString &text) {
	QString t = text;
	if(t.indexOf("^^") != -1) {
		// aliases
		t.replace("^^JID", account->username + "@" + account->host);
		t.replace("^^SERVER", account->host);
		t.replace("^^FJID", jid);
	}
	if(sstate == SSOK) {
		sendBuffer.write(t.toUtf8());
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
					writer.writeTextElement("group", d.getGroup());
					writer.writeEndElement();
				writer.writeEndElement();
			writer.writeEndElement();
			sendWriteBuffer();
		}
	}
}

void JabberCtx::removeRosterItem() {
	removeContact(mid);
}

void JabberCtx::removeContact(const QString &jid) {
	RosterItem *item = roster.get_item(jid);
	if(item) {
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

		sendRevoke(jid);
	}
}

void JabberCtx::sendGrant(const QString &to) {
	log("Granting subscription to " + to);
	writer.writeEmptyElement("presence");
	writer.writeAttribute("to", to);
	//writer.writeAttribute("from", jid); //Roster::full_jid2jid(jid));
	writer.writeAttribute("type", "subscribed");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::grantSubscription() {
	sendGrant(mid);
}

void JabberCtx::sendRevoke(const QString &to) {
	log("Revoking subscription from " + to);
	//<presence to='romeo@example.net' type='unsubscribed'/>
	writer.writeEmptyElement("presence");
	writer.writeAttribute("to", to);
	//writer.writeAttribute("from", jid); //Roster::full_jid2jid(jid));
	writer.writeAttribute("type", "unsubscribed");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::revokeSubscription() {
	sendRevoke(mid);
}

void JabberCtx::sendRequestSubscription(const QString &to) {
	log("Requesting subscription from " + to);
	writer.writeEmptyElement("presence");
	//writer.writeAttribute("from", Roster::full_jid2jid(jid));
	writer.writeAttribute("to", to);
	writer.writeAttribute("type", "subscribe");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::requestSubscription() {
	sendRequestSubscription(mid);
}

void JabberCtx::setPriority(int p) {
	priority = p;
	sendPresence();
}

bool JabberCtx::gatewayRegister(const QString &gateway) {
	log("Registering with gateway " + gateway);
	writer.writeStartElement("iq");
	writer.writeAttribute("type", "get");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("to", gateway);
	writer.writeAttribute("id", "gateway_register");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("jabber:iq:register");
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();	
	return true;
}

void JabberCtx::parseRegisterResult(const QString &gateway) {
	QString instructions;
	QStringList fields;
	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "query")) {
		readMoreIfNecessary();
		if(reader.isStartElement()) {
			if(reader.name() == "instructions")
				instructions = reader.readElementText();
			else if(reader.name() == "registered") {
				log("Already registered with gateway " + gateway, LMT_WARNING);
			} else if(reader.name() == "x") {
				while(reader.name() != "x" || !reader.isEndElement())
					readMoreIfNecessary();
			} else {
				fields << reader.name().toString();
			}
		}
	}

	GatewayRegister *r = new GatewayRegister(gateway, instructions, fields);
	connect(r, SIGNAL(gatewayRegistration(const QString &, const QMap<QString, QString> &)), this, SLOT(gatewayRegistration(const QString &, const QMap<QString, QString> &)));
	r->show();
}

void JabberCtx::gatewayRegistration(const QString &gateway, const QMap<QString, QString> &fields) {
	writer.writeStartElement("iq");
	writer.writeAttribute("type", "set");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("to", gateway);
	writer.writeAttribute("id", "gateway_register2");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("jabber:iq:register");
			foreach(QString field, fields.keys()) {
				writer.writeTextElement(field, fields[field]);
			}
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();	
}

bool JabberCtx::gatewayUnregister(const QString &gateway) {
	writer.writeStartElement("iq");
	writer.writeAttribute("type", "set");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("to", gateway);
	writer.writeAttribute("id", "gateway_unregister");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("jabber:iq:register");
			writer.writeEmptyElement("remove");
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();	

	removeContact(gateway);
	return true;
}
