#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QUrl>

class QWebSocket;

namespace qtautotest {

struct BridgeCallResult
{
    bool transportOk = false;
    QString transportError;
    QJsonObject response;
};

class BridgeClient : public QObject
{
    Q_OBJECT

public:
    explicit BridgeClient(QUrl bridgeUrl = QUrl(QStringLiteral("ws://127.0.0.1:49555")), QObject* parent = nullptr);
    ~BridgeClient() override;

    const QUrl& bridgeUrl() const;
    void setBridgeUrl(QUrl bridgeUrl);

    /// 同步调用桥接命令。
    /// 内部维护持久化 WebSocket 连接，首次调用时自动连接，断线自动重连。
    BridgeCallResult call(const QString& command, const QJsonObject& params = QJsonObject(),
                          int timeoutMs = 5000);

    /// 主动断开连接。下次 call() 时自动重连。
    void disconnectFromBridge();
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void transportError(const QString& message);

private:
    bool ensureConnected(int timeoutMs);
    void onTextMessageReceived(const QString& message);
    void onSocketError();

    QUrl m_bridgeUrl;
    QWebSocket* m_socket = nullptr;
    QHash<QString, QJsonObject> m_pendingResponses;
    QString m_lastError;
    bool m_connecting = false;
};

} // namespace qtautotest
