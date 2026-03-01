
#include "websockserver.h"
#include "QtWebSockets/qwebsocketserver.h"
#include "QtWebSockets/qwebsocket.h"
#include <QtCore/QDebug>

QT_USE_NAMESPACE

WebsockServer::WebsockServer(quint16 port, bool debug, QObject *parent) :
    QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("RobotBrowser DebugServer"),
                                            QWebSocketServer::NonSecureMode, this))
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, 8001)) {
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &WebsockServer::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebsockServer::closed);
    }
}

WebsockServer::~WebsockServer()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void WebsockServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebsockServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebsockServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebsockServer::socketDisconnected);

    pSocket->sendTextMessage("Connected to RobotBrowser Debugger!");

    m_clients << pSocket;
}

void WebsockServer::processTextMessage(QString message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient) {
        pClient->sendTextMessage(message);
    }
}

void WebsockServer::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());

    if (pClient) {
        pClient->sendBinaryMessage(message);
    }
}

void WebsockServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());

    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

void WebsockServer::sendWSMessage(QString message)
{
    foreach( QWebSocket *s, m_clients ) {
        // will items always be processed in numerical order by index?
        // do something with "item";
        s->sendTextMessage(message);
    }
}



