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
#include <QTextDocument> // for Qt::unescape function
#include <QFileDialog>
#include <QDesktopServices>

#include "newrosteritemdialog.h"
#include "gatewayregister.h"

QString unescape(const QString &s) {
	QString ret = s;
	ret.replace("&lt;", "<");
	ret.replace("&gt;", ">");
	ret.replace("&amp;", "&");
	return ret;
}

JabberCtx::JabberCtx(Account *acc, CoreI *core, QObject *parent)
        : QObject(parent), account(acc), useSSL(false), ignoreSSLErrors(false), core_i(core), writer(&sendBuffer),
                sstate(SSNONE), sessionRequired(false), tlsAvailable(false), tlsRequired(false),
				priority(DEFAULT_PRIORITY), ftId(1)
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

	menus_i = (MenusI *)core_i->get_interface(INAME_MENUS);
	if(menus_i) {
		fileTransferAction = menus_i->add_menu_action("Contact Menu", "Send File..");
		connect(fileTransferAction, SIGNAL(triggered()), this, SLOT(sendFile()));

		requestAction = menus_i->add_menu_action("Contact Menu", "Request");
		connect(requestAction, SIGNAL(triggered()), this, SLOT(requestSubscription()));

		revokeAction = menus_i->add_menu_action("Contact Menu", "Revoke");
		connect(revokeAction, SIGNAL(triggered()), this, SLOT(revokeSubscription()));

		grantAction = menus_i->add_menu_action("Contact Menu", "Grant");
		connect(grantAction, SIGNAL(triggered()), this, SLOT(grantSubscription()));

		removeRosterItemAction = menus_i->add_menu_action("Contact Menu", "Remove");
		connect(removeRosterItemAction, SIGNAL(triggered()), this, SLOT(removeRosterItem()));

		editRosterItemAction = menus_i->add_menu_action("Contact Menu", "Edit...");
		connect(editRosterItemAction, SIGNAL(triggered()), this, SLOT(editRosterItem()));
	}

	events_i = (EventsI *)core_i->get_interface(INAME_EVENTS);
	events_i->add_event_listener(this, UUID_SHOW_MENU);
	events_i->add_event_listener(this, UUID_CONTACT_CHANGED);
	events_i->add_event_listener(this, UUID_FT_USER);

	contact_info_i = (ContactInfoI*)core_i->get_interface(INAME_CONTACTINFO);

	keepAliveTimer.setInterval(30000);
	connect(&keepAliveTimer, SIGNAL(timeout()), this, SLOT(sendKeepAlive()));

	fileSendTimer.setInterval(100);
	connect(&fileSendTimer, SIGNAL(timeout()), this, SLOT(sendNextChunk()));
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

	events_i->remove_event_listener(this, UUID_SHOW_MENU);
	events_i->remove_event_listener(this, UUID_CONTACT_CHANGED);

	events_i->remove_event_listener(this, UUID_FT_USER);
}

Account *JabberCtx::get_account_info() {
	return account;
}

bool JabberCtx::event_fired(EventsI::Event &e) {
	if(e.uuid == UUID_SHOW_MENU) {
		ShowMenu &sm = static_cast<ShowMenu &>(e);
		if(sm.properties["name"] != "Contact Menu") return true;
		Contact *contact = contact_info_i->get_contact(sm.properties["ContactHashId"].toString());

		bool vis = (contact->account == account);
		//newRosterItemAction->setVisible(vis);

		removeRosterItemAction->setVisible(vis);
		editRosterItemAction->setVisible(vis);
		fileTransferAction->setVisible(vis);

		if(vis) {
			RosterItem *item = roster.get_item(contact->contact_id);
			SubscriptionType sub = item->getSubscription();

			bool to = (sub == ST_BOTH || sub == ST_TO),
				from = (sub == ST_BOTH || sub == ST_FROM);

			grantAction->setVisible(vis && !to);
			revokeAction->setVisible(vis && to);

			requestAction->setVisible(vis && !from);

			mid = contact->contact_id;
		} else {
			grantAction->setVisible(false);
			revokeAction->setVisible(false);

			requestAction->setVisible(false);
		}
	} else if(e.uuid == UUID_CONTACT_CHANGED && e.source != this) {
		ContactChanged &cc = static_cast<ContactChanged &>(e);
		if(cc.contact->account == account) {
			RosterItem *item = roster.get_item(cc.contact->contact_id);
			if(!item && !cc.removed) {
				cc.contact->mark_transient("name");
				cc.contact->mark_transient("group");
				cc.contact->mark_transient("status_msg");

				RosterItem *item = new RosterItem(cc.contact, "", ST_UNKNOWN, &roster);
				roster.addChild(item);
			} else if(item) {
				RosterGroup *g = item->getGroup();
				if(sstate == SSOK && g->getClistName() != cc.contact->get_property("group").toStringList()) {
					writer.writeStartElement("iq");
					writer.writeAttribute("type", "set");
					writer.writeAttribute("id", "roster_update");
						writer.writeStartElement("query");
						writer.writeDefaultNamespace("jabber:iq:roster");
							writer.writeStartElement("item");
							writer.writeAttribute("jid", cc.contact->contact_id);
							writer.writeAttribute("name", cc.contact->get_property("name").toString());
							writer.writeTextElement("group", cc.contact->get_property("group").toStringList().replaceInStrings(Roster::getDelimiter(), "|delim|").join(Roster::getDelimiter()));
							writer.writeEndElement();
						writer.writeEndElement();
					writer.writeEndElement();
					sendWriteBuffer();
				}
			}
		}
	} else if(e.uuid == UUID_FT_USER) {
		FileTransferUserEvent &ftue = (FileTransferUserEvent &)e;
		if(ftue.contact->account == account && ftue.type == EventsI::ET_OUTGOING) {
			if(ftue.ftType == FileTransferUserEvent::FT_REQUEST)
				requestFileTransfer(ftue.contact->contact_id, ftue.fileName, ftue.id);
			else if(ftue.ftType == FileTransferUserEvent::FT_ACCEPT)
				acceptFileTransfer(ftue.contact->contact_id, ftue.fileName, ftue.id);
			else if(ftue.ftType == FileTransferUserEvent::FT_CANCEL) {
				if(ftue.incoming && incomingTransfers[ftue.contact->contact_id].contains(ftue.id) && Roster::full_jid2jid(incomingTransfers[ftue.contact->contact_id][ftue.id].contact_id) == ftue.contact->contact_id) {
					rejectFileTransfer(incomingTransfers[ftue.contact->contact_id][ftue.id].contact_id, ftue.fileName, ftue.id);
				} else if(!ftue.incoming && outgoingTransfers.contains(ftue.id) && Roster::full_jid2jid(outgoingTransfers[ftue.id].contact_id) == ftue.contact->contact_id) {
					rejectFileTransfer(outgoingTransfers[ftue.id].contact_id, ftue.fileName, ftue.id);
				}
			}
		}
	}
	return true;
}

void JabberCtx::setUserChatState(Contact *contact, ChatStateType type) {
	RosterItem *item = roster.get_item(contact->contact_id);
	if(item && item->getUserChatState() != type) {
		item->setUserChatState(type);
		sendChatState(contact->contact_id, type);
	}
}

void JabberCtx::log(const QString &message, LogMessageType type) {
	QString msg = QString("Jabber (%1) - %2").arg(account->account_name).arg(message);
	switch(type) {
		case LMT_NORMAL:
			qDebug() << msg.toAscii().data();
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
			qWarning() << msg.toAscii().data();
			break;
		case LMT_ERROR:
			qCritical() << msg.toAscii().data();
			break;
		default:
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
			log("Disconnected");
			sslSocket.close();
			roster.clear();
			break;
		case SSSTARTSSL:
			setStatus(ST_CONNECTING);
			log("Connecting (encrypted)...");
			sslSocket.connectToHostEncrypted(connectionHost.isEmpty() ? account->host : connectionHost, account->port);
			break;
		case SSSTARTTLS:
			setStatus(ST_CONNECTING);
			log("Connecting...");
			sslSocket.connectToHost(connectionHost.isEmpty() ? account->host : connectionHost, account->port);
			break;
		case SSINITIALIZING:
			setStatus(ST_CONNECTING);
			log("Initializing...");
			startStream();
			break;
		case SSAUTHORIZING:
			setStatus(ST_CONNECTING);
			log("Authorizing...");
			authenticate();
			break;
		case SSLOGIN:
			setStatus(ST_CONNECTING);
			log("Getting roster...");
			startStream();
			break;
		case SSOK:
			setStatus(account->desiredStatus);
			log("Ok");
			//newRosterItemAction->setEnabled(true);
			keepAliveTimer.start();
			break;
		case SSTERMINATING:
			keepAliveTimer.stop();
			setStatus(ST_OFFLINE);
			log("Disconnecting...");
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
	log("Socket error:" + sslSocket.errorString(), LMT_WARNING);
	changeSessionState(SSNONE);
}

void JabberCtx::sslErrors(const QList<QSslError> &errors) {
	for(int i = 0; i < errors.size(); i++) {
		log("SSL error: " + errors.at(i).errorString(), (ignoreSSLErrors ? LMT_WARNING : LMT_ERROR));
	}
	if(ignoreSSLErrors)
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
				log("Error: " + reader.readElementText(), LMT_ERROR);
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
		} else if(reader.attributes().value("id") == "push") {
		} else if(outgoingTransfers.contains(id) && outgoingTransfers[id].contact_id == from) {
			readMoreIfNecessary();
			if(reader.isStartElement() && reader.name() == "si") {
				if(!outgoingTransfers[id].accepted) {
					outgoingTransfers[id].accepted = true;
					if(!fileSendTimer.isActive()) fileSendTimer.start();

					FileTransferUserEvent ftue(contact_info_i->get_contact(account, from), "", 0, id, this);
					ftue.type = EventsI::ET_INCOMING;
					ftue.ftType = FileTransferUserEvent::FT_ACCEPT;
					ftue.incoming = false;
					events_i->fire_event(ftue);


				} else
					log("Active transfer re-accepted", LMT_ERROR);
			} else if(outgoingTransfers[id].accepted) {
				outgoingTransfers[id].stream_accepted = true;
			}
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
		if(incomingTransfers[Roster::full_jid2jid(from)].contains(id) && incomingTransfers[Roster::full_jid2jid(from)][id].contact_id == from && reader.isStartElement()) {
			if(reader.name() == "open")
				parseIbbInit(id, from);
			else if(reader.name() == "data")
				parseIbbData(id, from);
			else if(reader.name() == "close")
				parseIbbClose(id, from);
		} else {
			if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:roster") {
				readMoreIfNecessary();
				parseRosterItem();
				sendEmptyResult(id, from);
			} else if(reader.isStartElement() && reader.name() == "si" && reader.namespaceUri() == "http://jabber.org/protocol/si") {
				parseFileTransferRequest(id, from);
			} else
				sendIqError(id, from);
		}
	} else if(reader.attributes().value("type") == "get") {
		readMoreIfNecessary();
		if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:version") {
			sendVersionInfoResult(id, from);
		} else if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "http://jabber.org/protocol/disco#info") {
			sendDiscoInfoResult(id, from);
		} else if(reader.isStartElement() && reader.name() == "query" && reader.namespaceUri() == "jabber:iq:time") {
			sendIqTimeResult(id, from);
		} else if(reader.isStartElement() && reader.name() == "time" && reader.namespaceUri() == "urn:xmpp:time") {
			sendXMPPTimeResult(id, from);
		} else {
			sendIqError(id, from);
		}
	} else if(reader.attributes().value("type") == "error") {
		if(reader.attributes().value("id") == "group_delimiter_get") {
			getRoster();
		} else if(incomingTransfers[Roster::full_jid2jid(from)].contains(id) && incomingTransfers[Roster::full_jid2jid(from)][id].contact_id == from) {
			delete incomingTransfers[Roster::full_jid2jid(from)][id].file;
			incomingTransfers[Roster::full_jid2jid(from)].remove(id);

			FileTransferUserEvent ftue(contact_info_i->get_contact(account, Roster::full_jid2jid(from)), "", 0, id, this);
			ftue.type = EventsI::ET_INCOMING;
			ftue.ftType = FileTransferUserEvent::FT_CANCEL;
			ftue.incoming = true;
			events_i->fire_event(ftue);
		} else if(outgoingTransfers.contains(id) && outgoingTransfers[id].contact_id == from) {
			delete outgoingTransfers[id].file;
			outgoingTransfers.remove(id);
			if(outgoingTransfers.size() == 0) fileSendTimer.stop();

			FileTransferUserEvent ftue(contact_info_i->get_contact(account, Roster::full_jid2jid(from)), "", 0, id, this);
			ftue.type = EventsI::ET_INCOMING;
			ftue.ftType = FileTransferUserEvent::FT_CANCEL;
			ftue.incoming = false;
			events_i->fire_event(ftue);
		} else {
			readMoreIfNecessary();
			if(reader.isStartElement() && reader.name() == "error") {
				if(reader.attributes().value("type") != "wait") {// error 'wait' code
					int code = reader.attributes().value("code").toString().toInt();
					QString msg = QString("Error (code %1): %2").arg(code);
					readMoreIfNecessary();
					if(reader.isStartElement() && reader.namespaceUri() == "urn:ietf:params:xml:ns:xmpp-stanzas") {
						log(msg.arg(reader.name().toString()), LMT_ERROR);
					} else
						log(msg.arg("unknown"), LMT_ERROR);
				}
			}
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
                default:
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
		readMoreIfNecessary();
		QString groupDelim = reader.text().toString();
		if(!groupDelim.isEmpty()) {
			RosterGroup::setDelimiter(groupDelim);
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

void JabberCtx::setDetails(RosterItem *item, const QStringList &group, const QString &name, SubscriptionType sub) {
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

void JabberCtx::addItem(const QString &jid, const QString &name, const QStringList &group, SubscriptionType sub) {
	RosterGroup *gr = roster.get_group(group);
	/*
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
	*/

	Contact *c = contact_info_i->get_contact(account, jid);
	c->mark_transient("name");
	c->mark_transient("group");
	c->mark_transient("status_msg");
	RosterItem *item = new RosterItem(c, name, sub, gr);
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
	QStringList gr_proper;

	readMoreIfNecessary();
	if(reader.isStartElement() && reader.name() == "group") {
		group = reader.readElementText();
		if(!group.isEmpty())
			gr_proper = group.split(Roster::getDelimiter()).replaceInStrings("|delim|", Roster::getDelimiter());
	}

	// ensure non-empty name
	if(name.isEmpty()) name = jid.split("@").at(0);

	RosterItem *item = roster.get_item(jid);
	if(subscription == "remove") {
		if(item) {
			item->getGroup()->removeChild(item);
			contact_info_i->delete_contact(item->getContact());
			delete item;
		}
	} else if(item) {
		setDetails(item, gr_proper, name, RosterItem::string2sub(subscription));
		if(ask == "subscribe") {
			//emit grantRequested(jid, account_id);
		} //if(subscription == "to")
			//sendRequestSubscription(jid);
	} else {
		//log("Adding id to roster: " + jid + "(group: " + group + ")");
		addItem(jid, name, gr_proper, RosterItem::string2sub(subscription));
		if(ask == "subscribe") {
			//emit grantRequested(jid, account_id);
		} //else if(subscription == "to")
			//sendRequestSubscription(jid);
	}
}

GlobalStatus presenceToStatus(PresenceType pt) {
	switch(pt) {
		case PT_UNAVAILABLE:	return ST_OFFLINE;
		case PT_INVISIBLE:		return ST_INVISIBLE;
                default:
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

	Resource *r = roster.get_resource(full_jid, true);
	if(Roster::full_jid2jid(full_jid) == full_jid) {
		item->setAllResourcePresence(presence, msg);
	} else {
		r->setPresence(presence);
		r->setPresenceMessage(msg);
		r->updateLastActivity();
		r->setPriority(prio);
	}
	// application contact status is based on 'active' resource
	r = item->get_active_resource();
	if(r) {
		item->getContact()->status = presenceToStatus(r->getPresence());
		if(r->getPresenceMessage().isEmpty())
			item->getContact()->remove_property("status_msg");
		else
			item->getContact()->set_property("status_msg", r->getPresenceMessage());
	}

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
		emit grantRequested(Roster::full_jid2jid(jid), account->account_id);
	} else if(presenceType == "subscribed") {
		//sendPresence(jid);
		//sendRequestSubscription(Roster::full_jid2jid(jid));
	} else if(presenceType == "unsubscribe") {
		//sendRevoke(Roster::full_jid2jid(jid));
	} else if(presenceType == "unsubscribed") {
		//sendRevoke(Roster::full_jid2jid(jid));
		//sendStopSubscription(Roster::full_jid2jid(jid));
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

	if(presenceType.isEmpty() || presenceType == "unavailable") {
		setPresence(jid, Resource::string2pres(presence), msg, prio);
	}

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

void JabberCtx::msgSend(Contact *contact, const QString &msg, int id) {
	writer.writeStartElement("message");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("to", contact->contact_id);
	writer.writeAttribute("type", "chat");
	writer.writeAttribute("id", QString("%1").arg(id));
	writer.writeAttribute("xml:lang", "en");
		writer.writeTextElement("body", msg);
		writer.writeEmptyElement("active");
		writer.writeDefaultNamespace("http://jabber.org/protocol/chatstates");
	writer.writeEndElement();
	sendWriteBuffer();
	log("Sent message to " + contact->contact_id);

	Message m(contact, msg, false, 0, this);
	events_i->fire_event(m);
}

void JabberCtx::parseMessageBody(const QString &source) {
	QString body = reader.readElementText();
	log("Received message from " + source);

	Resource *r = roster.get_resource(source,  false);
	if(r) {
		r->updateLastActivity();

		RosterItem *i = r->getItem();
		QString id = i->getJID();

		Message m(i->getContact(), body, true, 0, this);
		events_i->fire_event(m);
	} else {
		log("message from unknown resource ignored: " + source);
	}
}

void JabberCtx::parseMessage() {
	QString source = reader.attributes().value("from").toString(), 
		type = reader.attributes().value("type").toString(),
		body;
	if(type == "chat") {
		Resource *r = roster.get_resource(source,  false);
		if(r) {
			RosterItem *i = r->getItem();
			if(i) {
				ChatStateType state = CS_ACTIVE;
				while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "message")) {
					readMoreIfNecessary();
					if(!reader.atEnd() && reader.isStartElement() && reader.name() == "body") {
						parseMessageBody(source);
					}
					if(!reader.atEnd() && reader.isStartElement() && reader.name() == "active") {
						state = CS_ACTIVE;
					} else if(!reader.atEnd() && reader.isStartElement() && reader.name() == "inactive") {
						state = CS_INACTIVE;
					} else if(!reader.atEnd() && reader.isStartElement() && reader.name() == "composing") {
						state = CS_COMPOSING;
					} else if(!reader.atEnd() && reader.isStartElement() && reader.name() == "paused") {
						state = CS_PAUSED;
					} else if(!reader.atEnd() && reader.isStartElement() && reader.name() == "gone") {
						state = CS_GONE;
					}
				}
				ChatState cs(i->getContact(), state, true, this);
				events_i->fire_event(cs);
			}
		}
	} else {
		while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "message"))
			readMoreIfNecessary();
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
	writer.writeAttribute("from", jid);

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
	writer.writeAttribute("from", jid);
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
			writer.writeTextElement("name", core_i->platform());
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
			writer.writeEmptyElement("feature");
			writer.writeAttribute("var", "http://jabber.org/protocol/chatstates");
			writer.writeEmptyElement("feature");
			writer.writeAttribute("var", "jabber:iq:time");
			writer.writeEmptyElement("feature");
			writer.writeAttribute("var", "urn:xmpp:time");
			writer.writeEmptyElement("feature");
			writer.writeAttribute("var", "http://jabber.org/protocol/si");
			writer.writeEmptyElement("feature");
			writer.writeAttribute("var", "http://jabber.org/protocol/si/profile/file-transfer");
		writer.writeEndElement(); // query
	writer.writeEndElement(); // iq
	sendWriteBuffer();
	//log("sent version info to " + sender);
}

void JabberCtx::addContact(const QString &jid) {
	RosterItem *item = roster.get_item(jid);
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
		qWarning() << "JID" << jid << "already exists for account" << account->account_name;

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
		NewRosterItemDialog d(item->getJID(), item->getName(), g->getClistName());
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
					writer.writeTextElement("group", d.getGroup().replaceInStrings(Roster::getDelimiter(), "|delim|").join(Roster::getDelimiter()));
					writer.writeEndElement();
				writer.writeEndElement();
			writer.writeEndElement();
			sendWriteBuffer();
		}
	}
}

void JabberCtx::sendFile() {
	QString fn = QFileDialog::getOpenFileName(0, "Send File...", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));
	if(!fn.isEmpty()) {
		//requestFileTransfer(mid, fn, QString("ft_%1").arg(ftId++));
		FileTransferUserEvent ftue(contact_info_i->get_contact(account, mid), fn, QFile(fn).size(), QString("ft_%1").arg(ftId++), this);
		ftue.type = EventsI::ET_OUTGOING;
		ftue.ftType = FileTransferUserEvent::FT_REQUEST;
		ftue.incoming = false;
		events_i->fire_event(ftue);
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
	}
}

void JabberCtx::sendGrant(const QString &to) {
	RosterItem *item = roster.get_item(to);
	if(!item)
		addContact(to);

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
	RosterItem *item = roster.get_item(mid);
	if(!item)
		addContact(mid);
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
	log("Requesting subscription to " + to);
	writer.writeEmptyElement("presence");
	//writer.writeAttribute("from", Roster::full_jid2jid(jid));
	writer.writeAttribute("to", to);
	writer.writeAttribute("type", "subscribe");
	writer.writeCharacters("");
	sendWriteBuffer();
}

void JabberCtx::sendStopSubscription(const QString &to) {
	log("Stopping subscription from " + to);
	writer.writeEmptyElement("presence");
	//writer.writeAttribute("from", Roster::full_jid2jid(jid));
	writer.writeAttribute("to", to);
	writer.writeAttribute("type", "unsubscribe");
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
	QStringList fields, values;
	bool already_registered = false;
	while(!reader.atEnd() && !(reader.isEndElement() && reader.name() == "query")) {
		readMoreIfNecessary();
		if(reader.isStartElement()) {
			if(reader.name() == "instructions")
				instructions = reader.readElementText();
			else if(reader.name() == "registered") {
				already_registered = true;
				log("Already registered with gateway " + gateway);
			} else if(reader.name() == "x") {
				while(reader.name() != "x" || !reader.isEndElement())
					readMoreIfNecessary();
			} else {
				fields << reader.name().toString();
				values << reader.readElementText();
			}
		}
	}

	GatewayRegister *r = new GatewayRegister(gateway, instructions, fields, values);
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

void JabberCtx::sendChatState(const QString &id, ChatStateType type) {
	writer.writeStartElement("message");
	writer.writeAttribute("from", jid);
	writer.writeAttribute("to", id);
	writer.writeAttribute("type", "chat");
	writer.writeAttribute("xml:lang", "en");
		QString state;
		switch(type) {
			case CS_INACTIVE: state = "inactive"; break;
			case CS_ACTIVE: state = "active"; break;
			case CS_COMPOSING: state = "composing"; break;
			case CS_PAUSED: state = "paused"; break;
			case CS_GONE: state = "gone"; break;
		}
		writer.writeEmptyElement(state);
		writer.writeDefaultNamespace("http://jabber.org/protocol/chatstates");
	writer.writeEndElement();
	sendWriteBuffer();
	log("Sent chat state " + state + " to " + id);
}

void JabberCtx::sendIqTimeResult(const QString &id, const QString &sender) {
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	if(!sender.isEmpty())
		writer.writeAttribute("to", sender);
	writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "result");
		writer.writeStartElement("query");
		writer.writeDefaultNamespace("jabber:iq:time");

		QDateTime currentTime = QDateTime::currentDateTime();
		writer.writeTextElement("utc", currentTime.toUTC().toString("yyyyMMddTHH:mm:ss"));
		writer.writeTextElement("display", currentTime.toString());

		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
	log("Sent time (iq) to " + sender);
}

QString utcOffset() {
	QDateTime local = QDateTime::currentDateTime(),
		utc = local.toUTC();
	double off = 0;
	if(local.toString("d") != utc.toString("d")) off -= 24.0;
	double local_hours = local.toString("H").toInt() + local.toString("m").toInt() / 60.0,
		utc_hours = utc.toString("H").toInt() + utc.toString("m").toInt() / 60.0;

	off += local_hours - utc_hours;
	bool neg = (off < 0);
	if(neg) off *= -1;

	return QString().sprintf("%s%02d:%02d", (neg ? "-" : ""), (int)(off + 0.5), (int)((off - (int)(off + 0.5)) * 60 + 0.5));
}

void JabberCtx::sendXMPPTimeResult(const QString &id, const QString &sender) {
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	if(!sender.isEmpty())
		writer.writeAttribute("to", sender);
	writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "result");
		writer.writeStartElement("time");
		writer.writeDefaultNamespace("urn:xmpp:time");

		QDateTime currentTime = QDateTime::currentDateTime();
		writer.writeTextElement("utc", currentTime.toUTC().toString("yyyy-MM-ddThh:HH:ss'Z'"));
		writer.writeTextElement("tzo", utcOffset());

		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
	log("Sent time (xmpp) to " + sender);
}

void JabberCtx::requestFileTransfer(const QString &to, const QString &fileName, const QString &id) {
/*
<iq type='set' id='offer1' to='receiver@jabber.org/resource'>
  <si xmlns='http://jabber.org/protocol/si'
	  id='a0'
	  mime-type='text/plain'
	  profile='http://jabber.org/protocol/si/profile/file-transfer'>
	<file xmlns='http://jabber.org/protocol/si/profile/file-transfer'
		  name='test.txt'
		  size='1022'/>
	<feature xmlns='http://jabber.org/protocol/feature-neg'>
	  <x xmlns='jabber:x:data' type='form'>
		<field var='stream-method' type='list-single'>
		  <option><value>http://jabber.org/protocol/bytestreams</value></option>
		  <option><value>http://jabber.org/protocol/ibb</value></option>
		</field>
	  </x>
	</feature>
  </si>
</iq>
*/
	RosterItem *item = roster.get_item(to);
	Resource *r = item->get_active_resource();
	if(!r) return;

	QFile *f = new QFile(fileName);

	FTData data;
	data.id = id;
	data.stream_id = id;
	data.file = f;
	data.contact_id = r->full_jid();
	data.size = f->size();

	outgoingTransfers[id] = data;

	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	if(!to.isEmpty())
		writer.writeAttribute("to", r->full_jid());
	writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "set");

		writer.writeStartElement("si");
		writer.writeDefaultNamespace("http://jabber.org/protocol/si");
		writer.writeAttribute("id", id);
		writer.writeAttribute("profile", "http://jabber.org/protocol/si/profile/file-transfer");

			writer.writeStartElement("file");
			writer.writeDefaultNamespace("http://jabber.org/protocol/si/profile/file-transfer");
			writer.writeAttribute("name", fileName.mid(fileName.lastIndexOf("/") + 1));
			writer.writeAttribute("size", QString("%1").arg(f->size()));

				writer.writeStartElement("feature");
				writer.writeDefaultNamespace("http://jabber.org/protocol/feature-neg");

					writer.writeStartElement("x");
					writer.writeDefaultNamespace("jabber:x:data");
					writer.writeAttribute("type", "form");

						writer.writeStartElement("field");
						writer.writeAttribute("var", "stream-method");
						writer.writeAttribute("type", "list-single");

							writer.writeStartElement("option");

								writer.writeTextElement("value", "http://jabber.org/protocol/ibb");\

							writer.writeEndElement();
						writer.writeEndElement();
					writer.writeEndElement();
				writer.writeEndElement();
			writer.writeEndElement();
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
	log("Sent file transfer request (" + fileName + ") to " + to);
}

void JabberCtx::acceptFileTransfer(const QString &sender, const QString &fileName, const QString &id) {
/*
<iq type='result' to='sender@jabber.org/resource' id='offer1'>
  <si xmlns='http://jabber.org/protocol/si'>
	<feature xmlns='http://jabber.org/protocol/feature-neg'>
	  <x xmlns='jabber:x:data' type='submit'>
		<field var='stream-method'>
		  <value>http://jabber.org/protocol/bytestreams</value>
		</field>
	  </x>
	</feature>
  </si>
</iq>
*/
	if(!incomingTransfers[Roster::full_jid2jid(sender)].contains(id)) {
		return;
	}

	incomingTransfers[Roster::full_jid2jid(sender)][id].accepted = true;

	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	writer.writeAttribute("to", incomingTransfers[Roster::full_jid2jid(sender)][id].contact_id);
	writer.writeAttribute("from", jid);
	writer.writeAttribute("type", "result");

		writer.writeStartElement("si");
		writer.writeDefaultNamespace("http://jabber.org/protocol/si");

			writer.writeStartElement("feature");
			writer.writeDefaultNamespace("http://jabber.org/protocol/feature-neg");

				writer.writeStartElement("x");
				writer.writeDefaultNamespace("jabber:x:data");
				writer.writeAttribute("type", "submit");

					writer.writeStartElement("field");
					writer.writeAttribute("var", "stream-method");
						writer.writeTextElement("value", "http://jabber.org/protocol/ibb");\

					writer.writeEndElement();
				writer.writeEndElement();
			writer.writeEndElement();
		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();
	log("Sent file transfer acceptance (" + fileName + ") to " + sender);
}

void JabberCtx::rejectFileTransfer(const QString &sender, const QString &fileName, const QString &id) {
/*
<iq type='error' to='sender@jabber.org/resource' id='offer1'>
  <error code='403' type='cancel>
	<forbidden xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>
	<text xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'>Offer Declined</text>
  </error>
</iq>
*/
	writer.writeStartElement("iq");
	writer.writeAttribute("id", id);
	writer.writeAttribute("type", "error");
	writer.writeAttribute("to", sender);

		writer.writeStartElement("error");
		writer.writeAttribute("code", "403");
		writer.writeAttribute("type", "cancel");

			writer.writeEmptyElement("forbidden");
			writer.writeDefaultNamespace("urn:ietf:params:xml:ns:xmpp-stanzas");

			writer.writeTextElement("urn:ietf:params:xml:ns:xmpp-stanzas", "text", "Offer Declined");

		writer.writeEndElement();
	writer.writeEndElement();
	sendWriteBuffer();

	log("Sent file transfer rejection (" + fileName + ") to " + sender);

	if(incomingTransfers[Roster::full_jid2jid(sender)].contains(id) && sender == incomingTransfers[Roster::full_jid2jid(sender)][id].contact_id) {
		delete incomingTransfers[Roster::full_jid2jid(sender)][id].file;
		incomingTransfers[Roster::full_jid2jid(sender)].remove(id);
	}
	if(outgoingTransfers.contains(id) && sender == outgoingTransfers[id].contact_id) {
		delete outgoingTransfers[id].file;
		outgoingTransfers.remove(id);
	}
}

void JabberCtx::parseFileTransferRequest(const QString &id, const QString &from) {
	FTData ft;
	ft.id = id;
	ft.contact_id = from;
	ft.stream_id = reader.attributes().value("id").toString();

	do {
		readMoreIfNecessary();
		if(reader.isStartElement()) {
			if(reader.name() == "file") {
				QString fn = reader.attributes().value("name").toString();
				ft.file = new QFile(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/SajeDownloads/" + fn);
				ft.size = reader.attributes().value("size").toString().toUInt();

				//qDebug() << "Got file details, name is" << fn << "size is" << ft.size;
			//} else if(reader.name() == "feature") {
			} else
				log("skipping si element: " + reader.name().toString());
		}
	} while(!reader.atEnd() && reader.name() != "si");

	incomingTransfers[Roster::full_jid2jid(from)][id] = ft;

	FileTransferUserEvent ftue(contact_info_i->get_contact(account, Roster::full_jid2jid(from)), ft.file->fileName(), ft.size, id, this);
	ftue.type = EventsI::ET_INCOMING;
	ftue.ftType = FileTransferUserEvent::FT_REQUEST;
	ftue.incoming = true;
	events_i->fire_event(ftue);
}

void JabberCtx::sendNextChunk() {
	QMapIterator<QString, FTData> i(outgoingTransfers);
	foreach(QString id, outgoingTransfers.keys()) {
		FTData &data = outgoingTransfers[id];
		if(!data.started) {
			// initiate stream
/*
<iq from='romeo@montague.net/orchard'
	id='jn3h8g65'
	to='juliet@capulet.com/balcony'
	type='set'>
  <open xmlns='http://jabber.org/protocol/ibb'
		block-size='4096'
		sid='i781hf64'
		stanza='iq'/>
</iq>
*/
			if(data.file->open(QIODevice::ReadOnly)) {

				writer.writeStartElement("iq");
				writer.writeAttribute("id", data.id);
				writer.writeAttribute("type", "set");
				writer.writeAttribute("to", data.contact_id);
					writer.writeEmptyElement("open");
					writer.writeDefaultNamespace("http://jabber.org/protocol/ibb");
					writer.writeAttribute("block-size", QString("%1").arg(data.blockSize));
					writer.writeAttribute("sid", data.id);
					writer.writeAttribute("stanza", "iq");
				writer.writeEndElement();
				sendWriteBuffer();
				log("Sent ibb stream init to " + data.contact_id);

				data.started = true;
			} else {
				log("Failed to open file: " + data.file->fileName(), LMT_ERROR);
				sendIqError(data.id, jid, "cancel", "service-unavailable");
				delete data.file;
				outgoingTransfers.remove(data.id);
				return;
			}
		} else if(data.stream_accepted) {
			// send some data
/*
<iq from='romeo@montague.net/orchard'
	id='kr91n475'
	to='juliet@capulet.com/balcony'
	type='set'>
  <data xmlns='http://jabber.org/protocol/ibb' seq='0' sid='i781hf64'>
	qANQR1DBwU4DX7jmYZnncmUQB/9KuKBddzQH+tZ1ZywKK0yHKnq57kWq+RFtQdCJ
	WpdWpR0uQsuJe7+vh3NWn59/gTc5MDlX8dS9p0ovStmNcyLhxVgmqS8ZKhsblVeu
	IpQ0JgavABqibJolc3BKrVtVV1igKiX/N7Pi8RtY1K18toaMDhdEfhBRzO/XB0+P
	AQhYlRjNacGcslkhXqNjK5Va4tuOAPy2n1Q8UUrHbUd0g+xJ9Bm0G0LZXyvCWyKH
	kuNEHFQiLuCY6Iv0myq6iX6tjuHehZlFSh80b5BVV9tNLwNR5Eqz1klxMhoghJOA
  </data>
</iq>
*/
			if(data.progress == data.size && !data.completed) {
				// close stream
				writer.writeStartElement("iq");
				writer.writeAttribute("id", data.id);
				writer.writeAttribute("type", "set");
				writer.writeAttribute("to", data.contact_id);
				writer.writeAttribute("from", jid);
					writer.writeEmptyElement("http://jabber.org/protocol/ibb", "close");
					writer.writeAttribute("sid", data.stream_id);
				writer.writeEndElement();
				sendWriteBuffer();
				log("Sent stream close to " + data.contact_id);

				data.completed = true; // don't send lots of stream closes
			} else if(data.progress < data.size) {
				writer.writeStartElement("iq");
				writer.writeAttribute("id", data.id);
				writer.writeAttribute("type", "set");
				writer.writeAttribute("to", data.contact_id);
				writer.writeAttribute("from", jid);
					writer.writeStartElement("data");
					writer.writeDefaultNamespace("http://jabber.org/protocol/ibb");
					writer.writeAttribute("seq", QString("%1").arg(data.seq++));
					writer.writeAttribute("sid", data.stream_id);

					QByteArray bytes = data.file->read(data.blockSize);
					writer.writeCharacters(QString(bytes.toBase64()));
					data.progress += bytes.size();

					writer.writeEndElement();
				writer.writeEndElement();
				sendWriteBuffer();
				log("Sent stream data to " + data.contact_id);

				FileTransferProgress ftp(contact_info_i->get_contact(account, Roster::full_jid2jid(data.contact_id)), this);
				ftp.fileName = data.file->fileName();
				ftp.progressBytes = data.progress;
				ftp.sizeBytes = data.size;
				ftp.type = EventsI::ET_OUTGOING;
				ftp.id = data.id;
				events_i->fire_event(ftp);
			}
		}
	}
}

void JabberCtx::parseIbbInit(const QString &id, const QString &from) {
	QString stream_id = reader.attributes().value("sid").toString();
	int block_size = reader.attributes().value("block-size").toString().toInt();
	FTData &data = incomingTransfers[Roster::full_jid2jid(from)][id];
	data.stream_id = stream_id;
	data.blockSize = block_size;

	QDir saveDir(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/SajeDownloads");
	if(!saveDir.exists()) {
		if(!saveDir.mkdir(".")) {
			log("Failed create directory: " + saveDir.absolutePath());

			FileTransferUserEvent ftue(contact_info_i->get_contact(account, Roster::full_jid2jid(from)), "", 0, id, this);
			ftue.type = EventsI::ET_OUTGOING;
			ftue.ftType = FileTransferUserEvent::FT_CANCEL;
			ftue.incoming = true;
			events_i->fire_event(ftue);
			return;
		}
	}

	if(data.file->open(QIODevice::WriteOnly)) {
		writer.writeEmptyElement("iq");
		writer.writeAttribute("id", id);
		writer.writeAttribute("to", from);
		writer.writeAttribute("from", jid);
		writer.writeAttribute("type", "result");
		writer.writeCharacters("");
		sendWriteBuffer();
		log("Sent stream init accept to " + from);
	} else {
		log("Failed to open file: " + data.file->fileName());

		FileTransferUserEvent ftue(contact_info_i->get_contact(account, Roster::full_jid2jid(from)), "", 0, id, this);
		ftue.type = EventsI::ET_OUTGOING;
		ftue.ftType = FileTransferUserEvent::FT_CANCEL;
		ftue.incoming = true;
		events_i->fire_event(ftue);
	}
}

void JabberCtx::parseIbbData(const QString &id, const QString &from) {
	readMoreIfNecessary();
	QByteArray bytes = QByteArray::fromBase64(reader.text().toString().toAscii());
	FTData &data = incomingTransfers[Roster::full_jid2jid(from)][id];
	data.file->write(bytes);
	data.progress += bytes.size();

	qDebug() << "recv data: progress =" << data.progress << "size =" << data.size;

	FileTransferProgress ftp(contact_info_i->get_contact(account, Roster::full_jid2jid(data.contact_id)), this);
	ftp.fileName = data.file->fileName();
	ftp.progressBytes = data.progress;
	ftp.sizeBytes = data.size;
	ftp.type = EventsI::ET_INCOMING;
	ftp.id = data.id;
	events_i->fire_event(ftp);

	sendEmptyResult(id, from);
}

void JabberCtx::parseIbbClose(const QString &id, const QString &from) {
	FTData &data = incomingTransfers[Roster::full_jid2jid(from)][id];
	data.file->close();
	delete data.file;
	incomingTransfers[Roster::full_jid2jid(from)].remove(id);

	sendEmptyResult(id, from);
}
