#ifndef JABBERCTX_H
#define JABBERCTX_H

#include <QObject>
#include <QSslSocket>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMap>
#include <QMutex>
#include <QBuffer>
#include <QPointer>
#include <QTimer>
#include <QFile>
#include <QTimer>

#include "Roster.h"
#include <menus_i.h>
#include <accounts_i.h>
#include <filetransfer_i.h>
#include <global_status.h>
#include <QDomElement>
#include "disco.h"

typedef enum {LMT_NORMAL, LMT_WARNING, LMT_ERROR, LMT_SEND, LMT_RECV} LogMessageType;

#define DEFAULT_PRIORITY	48

class JabberCtx : public QObject, public EventsI::EventListener {
	Q_OBJECT

public:
	JabberCtx(Account *account, CoreI *core, QObject *parent = 0);
	~JabberCtx();
	
	void setAccountInfo(Account *acc);
	void log(const QString &message, LogMessageType type = LMT_NORMAL);

	GlobalStatus getCurrentStatus() {return account->status;}
	GlobalStatus getContactStatus(const QString &contact_id);

	void setUseSSL(bool f, bool ignore) {useSSL = f; ignoreSSLErrors = ignore;}
	bool getUseSSL() {return useSSL;}
	bool getIgnoreSSLErrors() {return ignoreSSLErrors;}

	QString getConnectionHost() {return connectionHost;}
	void setConnectionHost(const QString &host);

	bool directSend(const QString &text);
	bool gatewayRegister(const QString &gateway);
	bool gatewayUnregister(const QString &gateway);

	void setUserChatState(Contact *contact, ChatStateType type);

	Account *get_account_info();

	bool event_fired(EventsI::Event &e);
public slots:
	void msgSend(Contact *contact, const QString &msg, int id);
	void requestStatus(GlobalStatus gs);
	void addContact(const QString &jid);
	void removeContact(const QString &jid);

	void sendIqQueryDiscoInfo(const QString &entity_jid, const QString &node = "");
	void sendIqQueryDiscoItems(const QString &entity_jid, const QString &node = "");

	void sendGrant(const QString &jid);
	void sendRevoke(const QString &jid);

	void setPriority(int p);
signals:
	void msgRecv(const QString &account_id, const QString &jid, const QString &msg);
	void statusChanged(const QString &account_id, GlobalStatus gs);
	void contactStatusChanged(const QString &account_id, const QString &jid, GlobalStatus gs);
	void msgAck(int id);

	void gotDiscoInfo(const DiscoInfo &info);
	void gotDiscoItems(const DiscoItems &items);

	void gotGateway(const QString &account_id, const QString &gateway);
	void grantRequested(const QString &jid, const QString &account_id);

protected slots:
	void socketError(QAbstractSocket::SocketError socketError);
	void sslErrors(const QList<QSslError> &errors);

	void blockingReadSocketMore();
	void readMoreIfNecessary();

	void readSocket();
	void socketConnected();
	void socketEncrypted();
	void socketDisconnected();
	void socketStateChanged(QAbstractSocket::SocketState socketState);

	void connectToServer(bool con);

	// roster tree actions
	void addRosterItem();
	void editRosterItem();
	void removeRosterItem();

	void grantSubscription();
	void revokeSubscription();
	void requestSubscription();

	void sendRequestSubscription(const QString &to);
	void sendStopSubscription(const QString &to);

	//void setAllOffline();

	//void aboutToShowContactMenu(const QString &proto_name, const QString &account_id, const QString &id);
	//void aboutToShowGroupMenu(const QString &proto_name, const QString &account_id, const QString &full_gn);

	void gatewayRegistration(const QString &gateway, const QMap<QString, QString> &fields);

	void sendKeepAlive();

	void sendFile();

	void sendNextChunk();
protected:
	Account *account;
	bool useSSL, ignoreSSLErrors;
	QString connectionHost;

	QString to_clist_id(const QString &jid);
	bool is_ctx_id(const QString &clist_id);
	QString to_jid(const QString &clist_id);

	void setStatus(GlobalStatus gs);

	QString mid; // contact id for menu signals

	CoreI *core_i;
	QPointer<MenusI> menus_i;
	QPointer<EventsI> events_i;
	QPointer<ContactInfoI> contact_info_i;

	QAction *newRosterItemAction, *removeRosterItemAction, *editRosterItemAction,
		*grantAction, *revokeAction, *requestAction, *fileTransferAction;

	QSslSocket sslSocket;
	QXmlStreamReader reader;
	QBuffer sendBuffer;
	QXmlStreamWriter writer;
	void sendWriteBuffer();

	QTimer keepAliveTimer;

	enum SessionState {SSNONE, SSSTARTTLS, SSSTARTSSL, SSINITIALIZING, SSAUTHORIZING, SSTERMINATING, SSLOGIN, SSOK};

	void changeSessionState(const SessionState &newSate);

	QString sid;
	QString jid;
	SessionState sstate;
	bool sessionRequired, tlsAvailable, tlsRequired;
	int priority;

	void startStream();
	void endStream();

	void parseError();

	void parseStreamStart();
	void parseFeatures();
	QStringList mechs;
	void parseMechanisms();
	void parseChallenge();

	void parseFailure();
	void parseSuccess();
	void parseProceed();

	void parseIq();

	void start_tls();
	void authenticate();
	void bindResource();
	void startSession();
	void getGroupDelimiter();
	void getRoster();
	void sendPresence(const QString &to = QString());

	Roster roster;

	void parseGroupDelimiter();
	void parseRosterQuery();
	void parseRosterItem();
	void parsePresence();
	void parseMessage();
	void parseMessageBody(const QString &source);
	void parseDiscoInfoResult(const QString &entity);
	void parseDiscoItemsResult(const QString &entity);
	void parseRegisterResult(const QString &gateway);

	void setDetails(RosterItem *item, const QStringList &group, const QString &name, SubscriptionType sub);
	void addItem(const QString &jid, const QString &name, const QStringList &group, SubscriptionType sub);
	bool setPresence(const QString &full_jid, PresenceType presence, const QString &msg, int prio = 0);

	void sendIqError(const QString &id, const QString &sender, const QString &errorType = "cancel", const QString &definedCondition = "feature-not-implemented");

	void sendEmptyResult(const QString &id, const QString &sender);
	void sendVersionInfoResult(const QString &id, const QString &sender);
	void sendDiscoInfoResult(const QString &id, const QString &sender);

	void sendChatState(const QString &id, ChatStateType type);

	void sendIqTimeResult(const QString &id, const QString &sender);
	void sendXMPPTimeResult(const QString &id, const QString &sender);

	void requestFileTransfer(const QString &to, const QString &fileName, const QString &id);
	void acceptFileTransfer(const QString &sender, const QString &fileName, const QString &id);
	void rejectFileTransfer(const QString &sender, const QString &fileName, const QString &id);

	void parseFileTransferRequest(const QString &id, const QString &from);

	class FTData {
	public:
		FTData(): file(0), accepted(false), started(false), stream_accepted(false), completed(false), size(0), progress(0), seq(0), blockSize(4096) {}

		QString id, stream_id;
		QFile *file;
		QString contact_id;
		bool accepted, started, stream_accepted, completed;
		int size, progress, seq;
		int blockSize;
	};

	int ftId;

	QMap<QString, FTData> outgoingTransfers;
	QMap<QString, QMap<QString, FTData> > incomingTransfers; // id's might be the same for 2 contacts
	QTimer fileSendTimer;

	void parseIbbInit(const QString &id, const QString &from);
	void parseIbbData(const QString &id, const QString &from);
	void parseIbbClose(const QString &id, const QString &from);
};

#endif // JABBERCTX_H
