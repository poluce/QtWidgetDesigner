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
    QWebSocket* socket = nullptr;
    QString error;
};

BridgeStreamClient::BridgeStreamClient(QObject* parent)
    : AbstractBridgeClient(parent)
    , m_impl(new Impl())
{
    qRegisterMetaType<qtautotest::BridgeEvent>("qtautotest::BridgeEvent");

    m_impl->socket = new QWebSocket();
    m_socket = m_impl->socket;

    // 持久化信号连接
    QObject::connect(m_impl->socket, &QWebSocket::connected, this, [this]() {
        m_impl->error.clear();
        emit connected();
    });

    QObject::connect(m_impl->socket, &QWebSocket::disconnected, this, [this]() {
        emit disconnected();
    });

    QObject::connect(m_impl->socket, &QWebSocket::textMessageReceived, this,
                     [this](const QString& message) {
                         // 基类负责解析 JSON + 路由
                         AbstractBridgeClient::onTextMessageReceived(message);
                     });

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(m_impl->socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        m_impl->error = m_impl->socket->errorString();
        emit transportError(m_impl->error);
    });
#else
    QObject::connect(m_impl->socket,
                     QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     this,
                     [this](QAbstractSocket::SocketError) {
                         m_impl->error = m_impl->socket->errorString();
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
    // 手动清理 socket，基类析构时 m_socket == nullptr 会安全跳过
    // （基类析构已处理 m_destructing 路径，此处手动 delete 后置空即可）
    if (m_impl->socket != nullptr) {
        if (m_impl->socket->state() != QAbstractSocket::UnconnectedState) {
            m_impl->socket->close();
        }
        delete m_impl->socket;
        m_impl->socket = nullptr;
    }
    m_socket = nullptr;
    m_pendingResponses.clear();
    delete m_impl;
}

bool BridgeStreamClient::connectToBridge(int timeoutMs)
{
    return ensureConnected(timeoutMs);
}

void BridgeStreamClient::disconnectFromBridge()
{
    AbstractBridgeClient::disconnectFromBridge();
    // 基类已清理 m_socket 并置空，同步清理 m_impl->socket 防止悬挂
    m_impl->socket = nullptr;
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

    if (m_impl->socket->state() != QAbstractSocket::UnconnectedState) {
        m_impl->socket->abort();
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool connectedOk = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        m_impl->error = QStringLiteral("Timed out connecting to bridge.");
        m_impl->socket->abort();
        loop.quit();
    });

    QObject::connect(m_impl->socket, &QWebSocket::connected, &loop, [&]() {
        connectedOk = true;
        loop.quit();
    });

    QObject::connect(this, &BridgeStreamClient::transportError, &loop, [&]() {
        if (!connectedOk) {
            loop.quit();
        }
    });

    timer.start(timeoutMs > 0 ? timeoutMs : 5000);
    m_impl->socket->open(m_bridgeUrl);
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
