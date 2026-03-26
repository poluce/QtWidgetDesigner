#include <qtautotest/bridge_client.h>

#include <QAbstractSocket>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>
#include <QtGlobal>
#include <QUuid>
#include <QWebSocket>

namespace qtautotest {

BridgeClient::BridgeClient(QUrl bridgeUrl)
    : m_bridgeUrl(std::move(bridgeUrl))
{
}

const QUrl& BridgeClient::bridgeUrl() const
{
    return m_bridgeUrl;
}

BridgeCallResult BridgeClient::call(const QString& command, const QJsonObject& params, int timeoutMs) const
{
    BridgeCallResult result;

    QJsonObject request = params;
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.insert(QStringLiteral("id"), requestId);
    request.insert(QStringLiteral("command"), command);

    QWebSocket socket;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool responseReceived = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        result.transportError = QStringLiteral("Timed out waiting for bridge response.");
        socket.abort();
        loop.quit();
    });

    QObject::connect(&socket, &QWebSocket::connected, &loop, [&]() {
        const QJsonDocument document(request);
        socket.sendTextMessage(QString::fromUtf8(document.toJson(QJsonDocument::Compact)));
    });

    QObject::connect(&socket, &QWebSocket::textMessageReceived, &loop, [&](const QString& message) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            result.transportError = QStringLiteral("Bridge returned invalid JSON.");
            socket.abort();
            loop.quit();
            return;
        }

        const QJsonObject response = document.object();
        if (response.value(QStringLiteral("id")).toString() != requestId) {
            return;
        }

        responseReceived = true;
        result.transportOk = true;
        result.response = response;
        socket.close();
        loop.quit();
    });

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(&socket, &QWebSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
        if (responseReceived) {
            return;
        }
        result.transportError = socket.errorString();
        loop.quit();
    });
#else
    QObject::connect(&socket,
                     QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     &loop,
                     [&](QAbstractSocket::SocketError) {
                         if (responseReceived) {
                             return;
                         }
                         result.transportError = socket.errorString();
                         loop.quit();
                     });
#endif

    timer.start(timeoutMs > 0 ? timeoutMs : 5000);
    socket.open(m_bridgeUrl);
    loop.exec();

    if (!result.transportOk && result.transportError.isEmpty()) {
        result.transportError = QStringLiteral("Bridge request failed.");
    }

    return result;
}

} // namespace qtautotest
