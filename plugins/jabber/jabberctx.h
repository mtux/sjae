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

#include "Roster.h"
#include <clist_i.h>
#include <accounts_i.h>
#include <global_status.h>
#include <QDomElement>

typedef enum {LMT_NORMAL, LMT_WARNING, LMT_ERROR, LMT_SEND, LMT_RECV} LogMessageType;

class JabberCtx : public QObject
{
	Q_OBJECT

public:
	JabberCtx(const QString &account_id, const AccountInfo &info, CoreI *core, QObject *parent = 0);
	~JabberCtx();
	
	void setAccountInfo(const AccountInfo &info);
	void showMessage(const QString &message);
	void log(const QString &message, LogMessageType type = LMT_NORMAL);

	GlobalStatus getCurrentStatus() {return currentStatus;}

	void setUseSSL(bool f) {useSSL = f;}
	bool getUseSSL() {return useSSL;}

	QString getConnectionHost() {return connectionHost;}
	void setConnectionHost(const QString &host) {connectionHost = host;}
public slots:
	void msgSend(const QString &jid, const QString &msg, int id);
	void requestStatus(GlobalStatus gs);
	void addContact(const QString &jid);

signals:
	void msgRecv(const QString &account_id, const QString &jid, const QString &msg);
	void statusChanged(const QString &account_id, GlobalStatus gs);
	void msgAck(int id);

protected slots:
	void socketError(QAbstractSocket::SocketError socketError);
	void sslErrors(const QList<QSslError> &errors);

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

	void setAllOffline();

	void aboutToShowContactMenu(const QString &proto_name, const QString &account_id, const QString &id);
	void aboutToShowGroupMenu(const QString &proto_name, const QString &account_id, const QString &full_gn);

protected:
	AccountInfo acc_info;
	bool useSSL;
	QString connectionHost;

	QString to_clist_id(const QString &jid);
	bool is_ctx_id(const QString &clist_id);
	QString to_jid(const QString &clist_id);

	void setStatus(GlobalStatus gs);

	QString account_id;
	QString mid; // contact id for menu signals - set by aboutToShowContactMenu
	CoreI *core_i;
	QPointer<CListI> clist_i;

	QAction *newRosterItemAction, *removeRosterItemAction, *editRosterItemAction,
		*grantAction, *revokeAction, *requestAction;

	QSslSocket sslSocket;
	QXmlStreamReader reader;
	QBuffer sendBuffer;
	QXmlStreamWriter writer;
	void sendWriteBuffer();

	enum SessionState {SSNONE, SSSTARTTLS, SSSTARTSSL, SSINITIALIZING, SSAUTHORIZING, SSTERMINATING, SSLOGIN, SSOK};

	void changeSessionState(const SessionState &newSate);

	QString sid;
	QString jid;
	SessionState sstate;
	bool sessionRequired, tlsAvailable, tlsRequired;
	GlobalStatus connectStatus, currentStatus;

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
	void setDetails(RosterItem *item, const QString &group, const QString &name, SubscriptionType sub);
	void addItem(const QString &jid, const QString &name, const QString &group, SubscriptionType sub);
	bool setPresence(const QString &full_jid, PresenceType presence, const QString &msg);

	void sendIqError(const QString &id, const QString &sender, const QString &errorType = "cancel", const QString &definedCondition = "feature-not-implemented");

	void sendEmptyResult(const QString &id, const QString &sender);
	void sendVersionInfoResult(const QString &id, const QString &sender);
	void sendDiscoInfoResult(const QString &id, const QString &sender);

	void sendGrant(const QString &jid);
	void sendRevoke(const QString &jid);
};

#endif // JABBERCTX_H
