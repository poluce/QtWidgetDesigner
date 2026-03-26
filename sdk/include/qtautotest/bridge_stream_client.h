#pragma once

#include <qtautotest/bridge_client.h>

#include <QJsonArray>
#include <QJsonObject>
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

class BridgeStreamClient : public QObject
{
    Q_OBJECT

public:
    explicit BridgeStreamClient(QObject* parent = nullptr);
    explicit BridgeStreamClient(QUrl bridgeUrl, QObject* parent = nullptr);
    ~BridgeStreamClient() override;

    const QUrl& bridgeUrl() const;
    void setBridgeUrl(QUrl bridgeUrl);

    bool connectToBridge(int timeoutMs = 5000);
    void disconnectFromBridge();
    bool isConnected() const;

    QString errorString() const;

    BridgeCallResult call(const QString& command, const QJsonObject& params = QJsonObject(),
                          int timeoutMs = 5000);
    BridgeCallResult subscribe(const QStringList& events, const QJsonArray& selectors = QJsonArray(),
                               int debounceMs = 50, int timeoutMs = 5000);
    BridgeCallResult unsubscribe(const QString& subscriptionId, int timeoutMs = 5000);

signals:
    void connected();
    void disconnected();
    void eventReceived(const qtautotest::BridgeEvent& event);
    void transportError(const QString& message);

private:
    class Impl;
    Impl* m_impl = nullptr;
};

} // namespace qtautotest

Q_DECLARE_METATYPE(qtautotest::BridgeEvent)
