#include "bridge_client.h"

#include <QAbstractSocket>
#include <QEventLoop>
#include <QTimer>
#include <QWebSocket>

namespace qtautotest {

BridgeClient::BridgeClient(QUrl bridgeUrl, QObject* parent)
    : AbstractBridgeClient(parent)
{
    m_bridgeUrl = std::move(bridgeUrl);
}

BridgeClient::~BridgeClient() = default;

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
    // 注：使用 lambda 而非直接 PMF，因为在 Qt 5 中无法从派生类通过 PMF 访问基类 protected 成员
    QObject::connect(m_socket, &QWebSocket::textMessageReceived, this,
                     [this](const QString& message) {
                         AbstractBridgeClient::onTextMessageReceived(message);
                     });
    QObject::connect(m_socket, &QWebSocket::disconnected,
                     this, &AbstractBridgeClient::disconnected);

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

    QObject::connect(this, &AbstractBridgeClient::transportError, &loop, [&]() {
        if (!connectedOk) {
            loop.quit();
        }
    });

    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        m_lastError = QStringLiteral("Timed out connecting to bridge.");
        m_socket->abort();
        loop.quit();
    });

    timer.start(timeoutMs > 0 ? timeoutMs : 5000);
    m_socket->open(m_bridgeUrl);
    loop.exec();

    if (connectedOk) {
        emit connected();
        return true;
    }

    // 连接失败，清理 socket，下次重新创建
    m_socket->deleteLater();
    m_socket = nullptr;
    return false;
}

void BridgeClient::onSocketError()
{
    if (m_socket != nullptr) {
        m_lastError = m_socket->errorString();
    }
    emit transportError(m_lastError);
}

} // namespace qtautotest
