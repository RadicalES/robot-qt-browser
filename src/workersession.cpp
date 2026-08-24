#include "workersession.h"

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <QUrl>

namespace {

// Long enough for a server having a slow moment, short enough that an operator
// holding a card to a reader is not left watching "Signing on…" indefinitely.
// A worker who cannot get an answer needs to be told that quickly, because
// their next move is to call somebody.
constexpr int kTimeoutMs = 8000;

// Shorter, because this one is between a person pressing the power button and
// the terminal going off. A sign-off worth waiting for, but not for long.
constexpr int kShutdownTimeoutMs = 3000;

// Reads the wording the server sent back. A responseStation carries up to four
// LCD lines, meant for a terminal with a four-line display; here they are one
// message, which is the same information for a screen that has room for it.
QString lcdText(const QJsonObject& response)
{
    QStringList lines;
    for (const char* key : {"LCD1", "LCD2", "LCD3", "LCD4"}) {
        const QString line = response.value(QLatin1String(key)).toString().trimmed();
        if (!line.isEmpty())
            lines << line;
    }
    return lines.join(' ');
}

}  // namespace

WorkerSession::WorkerSession(QObject* parent)
    : QObject(parent)
{
    connect(&m_network, &QNetworkAccessManager::finished,
            this, &WorkerSession::onLogonReply);
}

QJsonObject WorkerSession::setup() const
{
    QFile file("/run/robot/setup.json");
    if (!file.open(QIODevice::ReadOnly))
        return QJsonObject();

    return QJsonDocument::fromJson(file.readAll()).object();
}

bool WorkerSession::isConfigured() const
{
    const QJsonObject config = setup();
    return !config.value("serverURL").toString().isEmpty()
            && !config.value("MAC").toString().isEmpty();
}

void WorkerSession::signOn(const QString& key)
{
    if (key.isEmpty())
        return;

    const QJsonObject config = setup();
    const QString url = config.value("serverURL").toString();
    if (url.isEmpty()) {
        // Not onboarded: there is nobody to ask. Saying so is the whole
        // message — an operator staring at a terminal that will not take their
        // card needs to know it is the terminal's fault, not their card's.
        emit unreachable(tr("This terminal is not registered with a server"));
        return;
    }

    m_pendingKey = key;

    QJsonObject logon;
    logon["MAC"] = config.value("MAC").toString();
    logon["id"] = key;
    logon["session"] = config.value("session").toString();

    QJsonObject payload;
    payload["publishLogon"] = logon;

    post(payload, true);
}

void WorkerSession::signOff()
{
    if (m_currentKey.isEmpty())
        return;

    const QJsonObject config = setup();
    const QString key = m_currentKey;
    m_currentKey.clear();

    if (!config.value("serverURL").toString().isEmpty()) {
        QJsonObject logoff;
        logoff["MAC"] = config.value("MAC").toString();
        logoff["session"] = config.value("session").toString();

        QJsonObject payload;
        payload["publishLogoff"] = logoff;

        post(payload, false);
    }

    qInfo() << "signed off:" << key;
    emit signedOff();
}

void WorkerSession::signOffBlocking()
{
    if (m_currentKey.isEmpty())
        return;

    const QJsonObject config = setup();
    const QString key = m_currentKey;
    m_currentKey.clear();

    const QString url = config.value("serverURL").toString();
    if (url.isEmpty()) {
        emit signedOff();
        return;
    }

    QJsonObject logoff;
    logoff["MAC"] = config.value("MAC").toString();
    logoff["session"] = config.value("session").toString();

    QJsonObject payload;
    payload["publishLogoff"] = logoff;

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(kShutdownTimeoutMs);

    QNetworkReply* reply =
            m_network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("expectReply", false);

    // A local event loop, because the application's is about to stop. Bounded
    // by the transfer timeout above, so a dead server delays a shutdown by
    // seconds rather than hanging it.
    QEventLoop wait;
    connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
    QTimer::singleShot(kShutdownTimeoutMs, &wait, &QEventLoop::quit);
    wait.exec();

    qInfo() << "signed off at shutdown:" << key
            << (reply->error() == QNetworkReply::NoError ? "(sent)" : "(not sent)");
    emit signedOff();
}

void WorkerSession::post(const QJsonObject& payload, bool expectReply)
{
    const QJsonObject config = setup();
    QNetworkRequest request{QUrl(config.value("serverURL").toString())};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(kTimeoutMs);

    // The reply is matched by this property rather than by keeping a pointer:
    // a sign-off is fire-and-forget and may well outlive the lock that caused
    // it, and its reply must not be read as a sign-on's answer.
    QNetworkReply* reply =
            m_network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("expectReply", expectReply);
}

void WorkerSession::onLogonReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (!reply->property("expectReply").toBool())
        return;   // a sign-off; nothing to decide

    const QString key = m_pendingKey;
    m_pendingKey.clear();

    if (reply->error() != QNetworkReply::NoError) {
        // Could not ask, which is not the same as being refused.
        //
        // A terminal that cannot reach its server does not let anybody on. It
        // was considered whether to sign a worker on anyway and queue the
        // event for later, so a switch reboot does not stop the line - and
        // rejected: a terminal that cannot check is a terminal that cannot
        // know, and letting a revoked worker through is a worse outcome than
        // making somebody wait for the network. Offline means offline.
        qWarning() << "sign-on could not reach the server:" << reply->errorString();
        emit unreachable(tr("Terminal offline — cannot sign on"));
        return;
    }

    const QJsonObject body =
            QJsonDocument::fromJson(reply->readAll()).object();
    const QJsonObject station = body.value("responseStation").toObject();
    const QString status = station.value("status").toString().trimmed().toUpper();
    const QString message = lcdText(station);

    if (status == "OK") {
        m_currentKey = key;
        qInfo() << "signed on:" << key;
        emit signedOn(key);
        return;
    }

    // DENIED, FAIL, LOGOFF — and anything the server invents later. All of
    // them mean the worker does not get in, and all of them come with the
    // server's own explanation, which is the one to show.
    if (status.isEmpty()) {
        qWarning() << "sign-on: no responseStation in the reply";
        emit unreachable(tr("The server did not answer properly"));
        return;
    }

    qInfo() << "sign-on refused:" << status << message;
    emit refused(message.isEmpty() ? tr("Not allowed at this terminal") : message);
}
