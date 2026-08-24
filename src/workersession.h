#ifndef WORKERSESSION_H
#define WORKERSESSION_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

class QNetworkReply;

// Signing a worker on and off at this terminal.
//
// A worker sign-on is **not** an authentication. It asks the server one
// question — is this operator allowed to use this terminal? — and takes the
// answer. No credentials are exchanged, no token is issued, and nothing is
// kept: the key a worker presents is an identity, not a secret. Staff signing
// in with a username and password is a different thing entirely, and is not
// this class.
//
// The exchange is the Robot API's Operator Sign On/Off, posted as JSON to the
// terminal's single server URI:
//
//     { "publishLogon"  : { "MAC": …, "id": "<worker key>", "session": … } }
//     { "publishLogoff" : { "MAC": …, "session": … } }
//
// and answered with a responseStation whose status decides:
//
//     { "responseStation": { "status": "OK/FAIL/DENIED/LOGOFF", "LCD1": … } }
//
// Everything needed to send it — MAC, session and serverURL — is already in
// /run/robot/setup.json, which robot-scada-client publishes. So this needs no
// new endpoint, no configuration of its own, and no change to the client.
class WorkerSession : public QObject {
    Q_OBJECT
public:
    explicit WorkerSession(QObject* parent = nullptr);

    // Whether this terminal knows where its server is. A terminal that has
    // never been onboarded has no setup.json, and cannot ask anybody anything.
    bool isConfigured() const;

    bool isSignedOn() const { return !m_currentKey.isEmpty(); }
    QString currentKey() const { return m_currentKey; }

public slots:
    // Asks the server whether this worker may use this terminal.
    void signOn(const QString& key);

    // Ends the session. Sent when the terminal locks — an operator who walks
    // away is off shift as far as the record is concerned, and the next person
    // to present a card is a new session.
    //
    // Deliberately fire-and-forget: a sign-off that failed because the network
    // dropped must not keep the terminal unlocked, and the server closes a
    // stale session on its own terms anyway.
    void signOff();

    // The same thing, but waiting for it to leave the machine.
    //
    // Only for shutdown. aboutToQuit is the last turn of the event loop, so a
    // POST queued there would be destroyed along with the network manager
    // before it was sent - and the session would stay open on the server for a
    // worker whose terminal is off. A short wait at shutdown is worth a
    // correct record.
    void signOffBlocking();

signals:
    void signedOn(const QString& key);
    void signedOff();

    // Refused by the server, with its own wording: the LCD lines a
    // responseStation carries are what an operator should be told, not a
    // phrase invented here that whoever they call has never heard.
    void refused(const QString& message);

    // Could not ask. A different state to being refused, and an operator will
    // be told the difference by whoever they call.
    void unreachable(const QString& message);

private slots:
    void onLogonReply(QNetworkReply* reply);

private:
    // Re-read per sign-on rather than cached: a terminal can be re-onboarded
    // while it is running, and the session id in particular changes when the
    // client re-registers with the server.
    QJsonObject setup() const;
    void post(const QJsonObject& payload, bool expectReply);

    QNetworkAccessManager m_network;
    QString m_currentKey;
    QString m_pendingKey;
};

#endif
