#pragma once

#include <QJsonObject>
#include <QUrl>

namespace qtautotest {

struct BridgeCallResult
{
    bool transportOk = false;
    QString transportError;
    QJsonObject response;
};

class BridgeClient
{
public:
    explicit BridgeClient(QUrl bridgeUrl = QUrl(QStringLiteral("ws://127.0.0.1:49555")));

    const QUrl& bridgeUrl() const;
    BridgeCallResult call(const QString& command, const QJsonObject& params = QJsonObject(),
                          int timeoutMs = 5000) const;

private:
    QUrl m_bridgeUrl;
};

} // namespace qtautotest
