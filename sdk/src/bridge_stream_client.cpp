#include "bridge_stream_client.h"

#include <QAbstractSocket>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QWebSocket>

namespace qtautotest {

class BridgeStreamClient::Impl
{
public:
    QWebSocket socket;
    QString error;
};

BridgeStreamClient::BridgeStreamClient(QObject* parent)
    : AbstractBridgeClient(parent)
    , m_impl(new Impl())
{
    qRegisterMetaType<qtautotest::BridgeEvent>("qtautotest::BridgeEvent");

    // 指向值成员，基类通过 m_socket 操作
    m_socket = &m_impl->socket;

    // 持久化信号连接
    QObject::connect(&m_impl->socket, &QWebSocket::connected, this, [this]() {
        m_impl->error.clear();
        emit connected();
    });

    QObject::connect(&m_impl->socket, &QWebSocket::disconnected, this, [this]() {
        emit disconnected();
    });

    QObject::connect(&m_impl->socket, &QWebSocket::textMessageReceived, this,
                     [this](const QString& message) {
                         // 基类负责解析 JSON + 路由
                         AbstractBridgeClient::onTextMessageReceived(message);
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
    // m_socket 指向 m_impl->socket（值成员），基类 disconnectFromBridge 会 deleteLater
    // 所以先断开 + 置空指针，让基类析构时跳过
    if (m_impl->socket.state() != QAbstractSocket::UnconnectedState) {
        m_impl->socket.close();
    }
    m_pendingResponses.clear();
    m_socket = nullptr;
    delete m_impl;
}

bool BridgeStreamClient::connectToBridge(int timeoutMs)
{
    return ensureConnected(timeoutMs);
}

QString BridgeStreamClient::errorString() const
{
    return m_impl->error;
}

bool BridgeStreamClient::ensureConnected(int timeoutMs)
{
    if (isConnected()) {
        return true;
    }

    m_impl->error.clear();
    m_pendingResponses.clear();

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
    m_impl->socket.open(m_bridgeUrl);
    loop.exec();

    return connectedOk;
}

bool BridgeStreamClient::handleJsonMessage(const QJsonObject& object)
{
    // 识别事件帧
    if (object.value(QStringLiteral("type")).toString() == QStringLiteral("event")) {
        BridgeEvent event;
        event.event = object.value(QStringLiteral("event")).toString();
        event.subscriptionId = object.value(QStringLiteral("subscriptionId")).toString();
        event.sequence = static_cast<qint64>(object.value(QStringLiteral("sequence")).toDouble());
        event.payload = object.value(QStringLiteral("payload")).toObject();
        emit eventReceived(event);
        return true; // 已处理，不再路由到 pendingResponses
    }

    return false; // 让基类按 id 存入 pendingResponses
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
