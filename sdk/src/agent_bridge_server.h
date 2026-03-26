#pragma once

#include <QJsonObject>
#include <QObject>

class QWebSocket;
class QWebSocketServer;

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
    void handleNewConnection();
    void handleTextMessage(QWebSocket* socket, const QString& message);
    void handleDisconnected(QWebSocket* socket);

    QJsonObject dispatch(const QJsonObject& request) const;
    QJsonObject successResponse(const QJsonValue& id, const QJsonObject& result) const;
    QJsonObject errorResponse(const QJsonValue& id, const QString& code, const QString& message,
                              const QJsonObject& details = QJsonObject()) const;

    QWebSocketServer* m_server = nullptr;
    QList<QWebSocket*> m_clients;
};
