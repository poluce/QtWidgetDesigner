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

/// 抽象基类：封装 BridgeClient / BridgeStreamClient 的公共 call() 逻辑。
///
/// 子类职责：
///   - 管理 QWebSocket 的生命周期（创建 / 连接 / 断开）
///   - 在收到文本消息时调用 onTextMessageReceived()
///   - 可选覆写 handleJsonMessage() 拦截事件帧等非响应消息
///
/// call() 使用 QEventLoop + QTimer 实现同步等待，由基类提供一次实现。
class AbstractBridgeClient : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AbstractBridgeClient)

public:
    explicit AbstractBridgeClient(QObject* parent = nullptr);
    ~AbstractBridgeClient() override;

    const QUrl& bridgeUrl() const;
    void setBridgeUrl(QUrl bridgeUrl);

    /// 当前是否已连接。
    bool isConnected() const;

    /// 断开连接并清理待响应队列。
    virtual void disconnectFromBridge();

    /// 同步调用桥接命令。
    /// 内部会先调用子类的 ensureConnected() 确保连接可用。
    /// 如果子类断线后没有自动重连，call() 会触发重连。
    BridgeCallResult call(const QString& command, const QJsonObject& params = QJsonObject(),
                          int timeoutMs = 5000);

signals:
    void connected();
    void disconnected();
    void transportError(const QString& message);

protected:
    /// 子类必须实现：确保 m_socket 处于已连接状态。
    /// 成功返回 true，失败返回 false 并设置 m_lastError。
    virtual bool ensureConnected(int timeoutMs) = 0;

    /// 子类收到 WebSocket 文本消息时调用此方法。
    /// 内部解析 JSON，然后调用 handleJsonMessage()。
    void onTextMessageReceived(const QString& message);

    /// 子类可覆写以拦截非响应消息（如事件帧）。
    /// 返回 true 表示已处理（不会进入 pendingResponses）。
    virtual bool handleJsonMessage(const QJsonObject& object);

    QWebSocket* m_socket = nullptr;
    QHash<QString, QJsonObject> m_pendingResponses;
    QString m_lastError;
    QUrl m_bridgeUrl;
    bool m_destructing = false;
};

} // namespace qtautotest
