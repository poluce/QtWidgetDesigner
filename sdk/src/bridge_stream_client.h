#pragma once

#include "abstract_bridge_client.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMetaType>
#include <QObject>
#include <QStringList>
#include <QUrl>

namespace qtautotest {

struct BridgeEvent
{
    QString event;
    QString subscriptionId;
    qint64 sequence = 0;
    QJsonObject payload;
};

/// 带有事件订阅能力的流式桥接客户端。
///
/// 内部维护持久化 WebSocket 连接（值成员），
/// 支持同步调用 + 异步事件帧接收。
class BridgeStreamClient : public AbstractBridgeClient
{
    Q_OBJECT

public:
    explicit BridgeStreamClient(QObject* parent = nullptr);
    explicit BridgeStreamClient(QUrl bridgeUrl, QObject* parent = nullptr);
    ~BridgeStreamClient() override;

    bool connectToBridge(int timeoutMs = 5000);

    QString errorString() const;

    BridgeCallResult subscribe(const QStringList& events, const QJsonArray& selectors = QJsonArray(),
                               int debounceMs = 50, int timeoutMs = 5000);
    BridgeCallResult unsubscribe(const QString& subscriptionId, int timeoutMs = 5000);

signals:
    void eventReceived(const qtautotest::BridgeEvent& event);

protected:
    bool ensureConnected(int timeoutMs) override;

    /// 覆写：识别事件帧 (type == "event")，存入 m_pendingResponses 之外的路径
    bool handleJsonMessage(const QJsonObject& object) override;

private:
    class Impl;
    Impl* m_impl = nullptr;
};

} // namespace qtautotest

Q_DECLARE_METATYPE(qtautotest::BridgeEvent)
