#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>

class QWebSocket;
class QWebSocketServer;
class UiEventMonitor;

class AgentBridgeServer : public QObject
{
    Q_OBJECT

public:
    explicit AgentBridgeServer(QObject* parent = nullptr);
    ~AgentBridgeServer() override;

    bool start(quint16 port);
    quint16 port() const;
    QString errorString() const;

private:
    struct EventSubscription
    {
        QString id;
        QPointer<QWebSocket> socket;
        QStringList events;
        QJsonArray selectors;
        int debounceMs = 50;
        QHash<QString, qint64> lastSentAtByFingerprint;
    };

    void handleNewConnection();
    void handleTextMessage(QWebSocket* socket, const QString& message);
    void handleDisconnected(QWebSocket* socket);

    void handleUiEvent(const QString& eventName, const QJsonObject& payload, const QStringList& relatedRefs);
    bool subscriptionMatches(const EventSubscription& subscription, const QString& eventName,
                             const QStringList& relatedRefs) const;
    void sendEventFrame(QWebSocket* socket, const QString& subscriptionId,
                        const QString& eventName, const QJsonObject& payload);

    QJsonObject dispatch(QWebSocket* socket, const QJsonObject& request);
    QJsonObject successResponse(const QJsonValue& id, const QJsonObject& result) const;
    QJsonObject errorResponse(const QJsonValue& id, const QString& code, const QString& message,
                              const QJsonObject& details = QJsonObject()) const;

    QWebSocketServer* m_server = nullptr;
    UiEventMonitor* m_eventMonitor = nullptr;
    QList<QWebSocket*> m_clients;
    QList<EventSubscription> m_subscriptions;
    QHash<QWebSocket*, quint64> m_nextSequenceBySocket;
    quint64 m_nextSubscriptionId = 1;
};
