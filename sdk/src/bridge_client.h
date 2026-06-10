#pragma once

#include "abstract_bridge_client.h"

class QWebSocket;

namespace qtautotest {

/// 轻量同步桥接客户端。
///
/// 每次通信自动管理 WebSocket 连接，支持断线自动重连。
/// 同步调用阻塞等待响应，适合 request-response 场景。
class BridgeClient : public AbstractBridgeClient
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(BridgeClient)

public:
    explicit BridgeClient(QUrl bridgeUrl = QUrl(QStringLiteral("ws://127.0.0.1:49555")), QObject* parent = nullptr);
    ~BridgeClient() override;

protected:
    bool ensureConnected(int timeoutMs) override;

private:
    void onSocketError();
};

} // namespace qtautotest
