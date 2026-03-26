#include <qtautotest/bridge_stream_client.h>

#include <QAbstractSocket>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>

namespace qtautotest {

class BridgeStreamClient::Impl
{
public:
    QUrl bridgeUrl = QUrl(QStringLiteral("ws://127.0.0.1:49555"));
    QWebSocket socket;
    QString error;
    QHash<QString, QJsonObject> responsesById;
};

namespace {

QString makeRequestId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace

BridgeStreamClient::BridgeStreamClient(QObject* parent)
    : QObject(parent)
    , m_impl(new Impl())
{
    qRegisterMetaType<qtautotest::BridgeEvent>("qtautotest::BridgeEvent");

    QObject::connect(&m_impl->socket, &QWebSocket::connected, this, [this]() {
        m_impl->error.clear();
        emit connected();
    });

    QObject::connect(&m_impl->socket, &QWebSocket::disconnected, this, [this]() {
        emit disconnected();
    });

    QObject::connect(&m_impl->socket, &QWebSocket::textMessageReceived, this, [this](const QString& message) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            m_impl->error = QStringLiteral("Bridge returned invalid JSON.");
            emit transportError(m_impl->error);
            return;
        }

        const QJsonObject object = document.object();
        if (object.value(QStringLiteral("type")).toString() == QStringLiteral("event")) {
            BridgeEvent event;
            event.event = object.value(QStringLiteral("event")).toString();
            event.subscriptionId = object.value(QStringLiteral("subscriptionId")).toString();
            event.sequence = static_cast<qint64>(object.value(QStringLiteral("sequence")).toDouble());
            event.payload = object.value(QStringLiteral("payload")).toObject();
            emit eventReceived(event);
            return;
        }

        const QString responseId = object.value(QStringLiteral("id")).toString();
        if (!responseId.isEmpty()) {
            m_impl->responsesById.insert(responseId, object);
        }
    });

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(&m_impl->socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        m_impl->error = m_impl->socket.errorString();
        emit transportError(m_impl->error);
    });
#else
    QObject::connect(&m_impl->socket,
                     QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     this,
                     [this](QAbstractSocket::SocketError) {
                         m_impl->error = m_impl->socket.errorString();
                         emit transportError(m_impl->error);
                     });
#endif
}

BridgeStreamClient::BridgeStreamClient(QUrl bridgeUrl, QObject* parent)
    : BridgeStreamClient(parent)
{
    setBridgeUrl(std::move(bridgeUrl));
}

BridgeStreamClient::~BridgeStreamClient()
{
    disconnectFromBridge();
    delete m_impl;
}

const QUrl& BridgeStreamClient::bridgeUrl() const
{
    return m_impl->bridgeUrl;
}

void BridgeStreamClient::setBridgeUrl(QUrl bridgeUrl)
{
    m_impl->bridgeUrl = std::move(bridgeUrl);
}

bool BridgeStreamClient::connectToBridge(int timeoutMs)
{
    if (isConnected()) {
        return true;
    }

    m_impl->error.clear();
    m_impl->responsesById.clear();

    if (m_impl->socket.state() != QAbstractSocket::UnconnectedState) {
        m_impl->socket.abort();
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool connectedOk = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        m_impl->error = QStringLiteral("Timed out connecting to bridge.");
        m_impl->socket.abort();
        loop.quit();
    });

    QObject::connect(&m_impl->socket, &QWebSocket::connected, &loop, [&]() {
        connectedOk = true;
        loop.quit();
    });

    QObject::connect(this, &BridgeStreamClient::transportError, &loop, [&]() {
        if (!connectedOk) {
            loop.quit();
        }
    });

    timer.start(timeoutMs > 0 ? timeoutMs : 5000);
    m_impl->socket.open(m_impl->bridgeUrl);
    loop.exec();

    return connectedOk;
}

void BridgeStreamClient::disconnectFromBridge()
{
    m_impl->responsesById.clear();
    if (m_impl->socket.state() == QAbstractSocket::UnconnectedState) {
        return;
    }
    m_impl->socket.close();
}

bool BridgeStreamClient::isConnected() const
{
    return m_impl->socket.state() == QAbstractSocket::ConnectedState;
}

QString BridgeStreamClient::errorString() const
{
    return m_impl->error;
}

BridgeCallResult BridgeStreamClient::call(const QString& command, const QJsonObject& params, int timeoutMs)
{
    BridgeCallResult result;

    if (!isConnected() && !connectToBridge(timeoutMs)) {
        result.transportError = errorString();
        return result;
    }

    QJsonObject request = params;
    const QString requestId = makeRequestId();
    request.insert(QStringLiteral("id"), requestId);
    request.insert(QStringLiteral("command"), command);

    const QJsonDocument document(request);
    m_impl->socket.sendTextMessage(QString::fromUtf8(document.toJson(QJsonDocument::Compact)));

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        result.transportError = QStringLiteral("Timed out waiting for bridge response.");
        loop.quit();
    });

    QObject::connect(this, &BridgeStreamClient::transportError, &loop, [&]() {
        if (result.transportError.isEmpty()) {
            result.transportError = errorString();
        }
        loop.quit();
    });

    QObject::connect(&m_impl->socket, &QWebSocket::disconnected, &loop, [&]() {
        if (result.transportError.isEmpty()) {
            result.transportError = QStringLiteral("Bridge connection was closed.");
        }
        loop.quit();
    });

    QObject::connect(&m_impl->socket, &QWebSocket::textMessageReceived, &loop, [&](const QString&) {
        if (m_impl->responsesById.contains(requestId)) {
            loop.quit();
        }
    });

    timer.start(timeoutMs > 0 ? timeoutMs : 5000);
    loop.exec();

    if (m_impl->responsesById.contains(requestId)) {
        result.transportOk = true;
        result.response = m_impl->responsesById.take(requestId);
    } else if (result.transportError.isEmpty()) {
        result.transportError = QStringLiteral("Bridge request failed.");
    }

    return result;
}

BridgeCallResult BridgeStreamClient::subscribe(const QStringList& events, const QJsonArray& selectors,
                                               int debounceMs, int timeoutMs)
{
    QJsonObject params{
        {QStringLiteral("events"), QJsonArray::fromStringList(events)},
        {QStringLiteral("selectors"), selectors},
        {QStringLiteral("debounceMs"), debounceMs},
    };
    return call(QStringLiteral("subscribe"), params, timeoutMs);
}

BridgeCallResult BridgeStreamClient::unsubscribe(const QString& subscriptionId, int timeoutMs)
{
    return call(QStringLiteral("unsubscribe"),
                QJsonObject{{QStringLiteral("subscriptionId"), subscriptionId}}, timeoutMs);
}

} // namespace qtautotest
