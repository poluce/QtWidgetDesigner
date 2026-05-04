#include "bridge_client.h"

#include <QAbstractSocket>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>

namespace qtautotest {

BridgeClient::BridgeClient(QUrl bridgeUrl, QObject* parent)
    : QObject(parent)
    , m_bridgeUrl(std::move(bridgeUrl))
{
}

BridgeClient::~BridgeClient()
{
    disconnectFromBridge();
}

const QUrl& BridgeClient::bridgeUrl() const
{
    return m_bridgeUrl;
}

void BridgeClient::setBridgeUrl(QUrl bridgeUrl)
{
    if (m_socket != nullptr && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->close();
    }
    m_bridgeUrl = std::move(bridgeUrl);
}

bool BridgeClient::isConnected() const
{
    return m_socket != nullptr && m_socket->state() == QAbstractSocket::ConnectedState;
}

void BridgeClient::disconnectFromBridge()
{
    if (m_socket != nullptr) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_pendingResponses.clear();
    m_connecting = false;
}

bool BridgeClient::ensureConnected(int timeoutMs)
{
    if (isConnected()) {
        return true;
    }

    // 清理残留状态
    if (m_socket != nullptr) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_pendingResponses.clear();
    m_lastError.clear();

    m_socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    // 持久化连接信号（只连接一次）
    QObject::connect(m_socket, &QWebSocket::textMessageReceived,
                     this, &BridgeClient::onTextMessageReceived);
    QObject::connect(m_socket, &QWebSocket::disconnected,
                     this, &BridgeClient::disconnected);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(m_socket, &QWebSocket::errorOccurred,
                     this, &BridgeClient::onSocketError);
#else
    QObject::connect(m_socket,
                     QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     this, &BridgeClient::onSocketError);
#endif

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool connectedOk = false;

    QObject::connect(m_socket, &QWebSocket::connected, &loop, [&]() {
        connectedOk = true;
        loop.quit();
    });

    QObject::connect(m_socket, &QWebSocket::disconnected, &loop, [&]() {
        if (!connectedOk) {
            m_lastError = QStringLiteral("WebSocket connection was closed before fully connected.");
            loop.quit();
        }
    });

    QObject::connect(this, &BridgeClient::transportError, &loop, [&]() {
        if (!connectedOk) {
            loop.quit();
        }
    });

    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        m_lastError = QStringLiteral("Timed out connecting to bridge.");
        m_socket->abort();
        loop.quit();
    });

    m_connecting = true;
    timer.start(timeoutMs > 0 ? timeoutMs : 5000);
    m_socket->open(m_bridgeUrl);
    loop.exec();
    m_connecting = false;

    if (connectedOk) {
        emit connected();
        return true;
    }

    // 连接失败，清理 socket，下次重新创建
    m_socket->deleteLater();
    m_socket = nullptr;
    return false;
}

void BridgeClient::onTextMessageReceived(const QString& message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        m_lastError = QStringLiteral("Bridge returned invalid JSON.");
        emit transportError(m_lastError);
        return;
    }

    const QJsonObject object = document.object();
    const QString responseId = object.value(QStringLiteral("id")).toString();
    if (!responseId.isEmpty()) {
        m_pendingResponses.insert(responseId, object);
    }
}

void BridgeClient::onSocketError()
{
    if (m_socket != nullptr) {
        m_lastError = m_socket->errorString();
    }
    emit transportError(m_lastError);
}

BridgeCallResult BridgeClient::call(const QString& command, const QJsonObject& params, int timeoutMs)
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

    // 错误也要退出（旧代码行为，防止错误非断线场景）
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

} // namespace qtautotest
