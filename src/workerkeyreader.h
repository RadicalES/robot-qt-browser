#ifndef WORKERKEYREADER_H
#define WORKERKEYREADER_H

#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

// A worker's key, arriving from a card reader or a barcode scanner.
//
// Both are the same thing here. The terminal's readers are bridged to a
// websocket by the wsrobot service — one server, however many readers — so a
// scan is a line of text arriving unprompted on ws://localhost:8100. What kind
// of reader produced it is a property of the terminal's wiring, not of the
// login: a worker presents a card or scans a badge and the same digits arrive
// either way.
//
// This connects only while the lock screen is showing. A terminal that is
// unlocked has no business holding the reader open — the page may want it for
// transactions, and wsrobot closes a port when the last client goes away.
class WorkerKeyReader : public QObject {
    Q_OBJECT
public:
    explicit WorkerKeyReader(const QUrl& url, QObject* parent = nullptr)
        : QObject(parent), m_url(url)
    {
        connect(&m_socket, &QWebSocket::textMessageReceived,
                this, &WorkerKeyReader::onMessage);
        connect(&m_socket, &QWebSocket::connected, this, [this]() {
            m_retry.stop();
            emit availabilityChanged(true);
        });
        connect(&m_socket, &QWebSocket::disconnected, this, [this]() {
            emit availabilityChanged(false);
            // Reconnect while we are meant to be listening: the bridge is
            // restarted whenever somebody saves the Card Reader page, and a
            // lock screen that stopped reading cards at that moment would look
            // like a broken reader.
            if (m_listening)
                m_retry.start();
        });

        m_retry.setInterval(3000);
        m_retry.setSingleShot(false);
        connect(&m_retry, &QTimer::timeout, this, [this]() {
            if (m_listening && m_socket.state() == QAbstractSocket::UnconnectedState)
                m_socket.open(m_url);
        });
    }

    ~WorkerKeyReader() override { stop(); }

    bool isConnected() const
    {
        return m_socket.state() == QAbstractSocket::ConnectedState;
    }

    // Strips what the bridge adds, leaving the key the worker presented.
    //
    // A terminal with more than one reader — the ITPC-200 has two — is
    // configured with "[%p]%s", so a card arrives as "[0]19045409306". That is
    // a card number with decoration, not a different card. Any leading
    // "[...]" is removed, and so is a "[CARD]:" style prefix, which some
    // deployments use instead.
    static QString extractKey(const QString& message)
    {
        QString text = message.trimmed();

        static const QRegularExpression prefix(QStringLiteral("^\\[[^\\]]*\\]\\s*:?\\s*"));
        text.remove(prefix);

        return text.trimmed();
    }

public slots:
    void start()
    {
        if (m_listening)
            return;

        m_listening = true;
        m_socket.open(m_url);
        m_retry.start();
    }

    void stop()
    {
        m_listening = false;
        m_retry.stop();
        if (m_socket.state() != QAbstractSocket::UnconnectedState)
            m_socket.close();
    }

signals:
    void keyRead(const QString& key);
    void availabilityChanged(bool available);

private slots:
    void onMessage(const QString& message)
    {
        const QString key = extractKey(message);
        if (key.isEmpty())
            return;

        // Readers announce themselves when a port opens — the ITPC-200's send
        // "<ITPC200 READER 0, Version 1.3>" — and that is not a card.
        if (key.startsWith('<') && key.endsWith('>'))
            return;

        emit keyRead(key);
    }

private:
    QUrl m_url;
    QWebSocket m_socket;
    QTimer m_retry;
    bool m_listening = false;
};

#endif
