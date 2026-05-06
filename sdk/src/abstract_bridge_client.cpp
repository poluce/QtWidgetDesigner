#include "abstract_bridge_client.h"

#include <QAbstractSocket>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>

namespace qtautotest {

AbstractBridgeClient::AbstractBridgeClient(QObject* parent)
    : QObject(parent)
{
}

AbstractBridgeClient::~AbstractBridgeClient()
{
    disconnectFromBridge();
}

const QUrl& AbstractBridgeClient::bridgeUrl() const
{
    return m_bridgeUrl;
}

void AbstractBridgeClient::setBridgeUrl(QUrl bridgeUrl)
{
    if (m_socket != nullptr && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->close();
    }
    m_bridgeUrl = std::move(bridgeUrl);
}

bool AbstractBridgeClient::isConnected() const
{
    return m_socket != nullptr && m_socket->state() == QAbstractSocket::ConnectedState;
}

void AbstractBridgeClient::disconnectFromBridge()
{
    if (m_socket != nullptr) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_pendingResponses.clear();
    m_connecting = false;
}

BridgeCallResult AbstractBridgeClient::call(const QString& command, const QJsonObject& params, int timeoutMs)
{
    BridgeCallResult result;

    // 确保已连接（首次调用自动连接，断线自动重连）
    if (!ensureConnected(timeoutMs)) {
        result.transportError = m_lastError;
        return result;
    }

    // 构造请求
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject request = params;
    request.insert(QStringLiteral("id"), requestId);
    request.insert(QStringLiteral("command"), command);

    // 发送请求
    const QJsonDocument document(request);
    m_socket->sendTextMessage(QString::fromUtf8(document.toJson(QJsonDocument::Compact)));

    // 等待匹配的响应（无锁，安全的事件循环等待）
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool responseReceived = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        result.transportError = QStringLiteral("Timed out waiting for bridge response.");
        loop.quit();
    });

    // 监听新消息到达，检查是否匹配我们的 requestId
    QMetaObject::Connection conn = QObject::connect(
        m_socket, &QWebSocket::textMessageReceived, &loop, [&](const QString&) {
            if (m_pendingResponses.contains(requestId)) {
                responseReceived = true;
                loop.quit();
            }
        });

    // 断线也要退出（防止死等）
    QObject::connect(m_socket, &QWebSocket::disconnected, &loop, [&]() {
        if (!responseReceived) {
            result.transportError = QStringLiteral("Bridge connection was closed.");
            loop.quit();
        }
    });

    // 错误也要退出
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(m_socket, &QWebSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
#else
    QObject::connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), &loop, [&](QAbstractSocket::SocketError) {
#endif
        if (!responseReceived) {
            result.transportError = m_socket->errorString();
            loop.quit();
        }
    });

    timer.start(timeoutMs > 0 ? timeoutMs : 5000);
    loop.exec();

    // 断开本次等待的临时连接
    QObject::disconnect(conn);

    if (responseReceived && m_pendingResponses.contains(requestId)) {
        result.transportOk = true;
        result.response = m_pendingResponses.take(requestId);
    } else if (result.transportError.isEmpty()) {
        result.transportError = QStringLiteral("Bridge request failed.");
    }

    return result;
}

void AbstractBridgeClient::onTextMessageReceived(const QString& message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        m_lastError = QStringLiteral("Bridge returned invalid JSON.");
        emit transportError(m_lastError);
        return;
    }

    const QJsonObject object = document.object();

    // 子类可先拦截（如事件帧），如果返回 true 则不再路由到 pendingResponses
    if (handleJsonMessage(object)) {
        return;
    }

    // 默认：按 id 存入待响应队列
    const QString responseId = object.value(QStringLiteral("id")).toString();
    if (!responseId.isEmpty()) {
        m_pendingResponses.insert(responseId, object);
    }
}

bool AbstractBridgeClient::handleJsonMessage(const QJsonObject& object)
{
    Q_UNUSED(object);
    return false; // 默认不拦截
}

} // namespace qtautotest
