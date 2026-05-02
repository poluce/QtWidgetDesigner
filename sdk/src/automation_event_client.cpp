#include <qtautotest/automation_event_client.h>

#include "automation_codec.h"
#include "bridge_stream_client.h"

#include <utility>

namespace qtautotest {

class AutomationEventClient::Impl
{
public:
    QUrl bridgeUrl = QUrl(QStringLiteral("ws://127.0.0.1:49555"));
    BridgeStreamClient streamClient;
    QString error;
    std::function<void()> connectedHandler;
    std::function<void()> disconnectedHandler;
    std::function<void(const QString&)> transportErrorHandler;
    std::function<void(const TabChangedEvent&)> tabChangedHandler;
    std::function<void(const ActivePageChangedEvent&)> activePageChangedHandler;
    std::function<void(const WindowFocusChangedEvent&)> windowFocusChangedHandler;
    std::function<void(const FocusWidgetChangedEvent&)> focusWidgetChangedHandler;
    std::function<void(const ModalDialogChangedEvent&)> modalDialogChangedHandler;
};

namespace {

template <typename T>
Result<T> transportFailure(const QString& message)
{
    Result<T> result;
    result.error = detail::parseAutomationError(QJsonObject{}, message);
    return result;
}

template <typename T>
Result<T> bridgeFailure(const QJsonObject& response)
{
    Result<T> result;
    result.error = detail::parseAutomationError(response);
    return result;
}

OperationResult operationTransportFailure(const QString& message)
{
    OperationResult result;
    result.error = detail::parseAutomationError(QJsonObject{}, message);
    return result;
}

OperationResult operationBridgeFailure(const QJsonObject& response)
{
    OperationResult result;
    result.error = detail::parseAutomationError(response);
    return result;
}

} // namespace

AutomationEventClient::AutomationEventClient()
    : m_impl(new Impl())
{
    QObject::connect(&m_impl->streamClient, &BridgeStreamClient::connected, &m_impl->streamClient,
                     [this]() {
        if (m_impl->connectedHandler) {
            m_impl->connectedHandler();
        }
    });
    QObject::connect(&m_impl->streamClient, &BridgeStreamClient::disconnected, &m_impl->streamClient,
                     [this]() {
        if (m_impl->disconnectedHandler) {
            m_impl->disconnectedHandler();
        }
    });
    QObject::connect(&m_impl->streamClient, &BridgeStreamClient::transportError, &m_impl->streamClient,
                     [this](const QString& message) {
        m_impl->error = message;
        if (m_impl->transportErrorHandler) {
            m_impl->transportErrorHandler(message);
        }
    });
    QObject::connect(&m_impl->streamClient, &BridgeStreamClient::eventReceived, &m_impl->streamClient,
                     [this](const BridgeEvent& event) {
        switch (detail::eventKindFromString(event.event)) {
        case EventKind::TabChanged:
            if (m_impl->tabChangedHandler) {
                m_impl->tabChangedHandler(detail::parseTabChangedEvent(event.subscriptionId, event.sequence, event.payload));
            }
            break;
        case EventKind::ActivePageChanged:
            if (m_impl->activePageChangedHandler) {
                m_impl->activePageChangedHandler(detail::parseActivePageChangedEvent(event.subscriptionId, event.sequence, event.payload));
            }
            break;
        case EventKind::WindowFocusChanged:
            if (m_impl->windowFocusChangedHandler) {
                m_impl->windowFocusChangedHandler(detail::parseWindowFocusChangedEvent(event.subscriptionId, event.sequence, event.payload));
            }
            break;
        case EventKind::FocusWidgetChanged:
            if (m_impl->focusWidgetChangedHandler) {
                m_impl->focusWidgetChangedHandler(detail::parseFocusWidgetChangedEvent(event.subscriptionId, event.sequence, event.payload));
            }
            break;
        case EventKind::ModalDialogChanged:
            if (m_impl->modalDialogChangedHandler) {
                m_impl->modalDialogChangedHandler(detail::parseModalDialogChangedEvent(event.subscriptionId, event.sequence, event.payload));
            }
            break;
        case EventKind::Unknown:
            break;
        }
    });
}

AutomationEventClient::AutomationEventClient(QUrl bridgeUrl)
    : AutomationEventClient()
{
    setBridgeUrl(std::move(bridgeUrl));
}

AutomationEventClient::~AutomationEventClient()
{
    disconnectFromBridge();
    delete m_impl;
}

const QUrl& AutomationEventClient::bridgeUrl() const
{
    return m_impl->bridgeUrl;
}

void AutomationEventClient::setBridgeUrl(QUrl bridgeUrl)
{
    m_impl->bridgeUrl = std::move(bridgeUrl);
    m_impl->streamClient.setBridgeUrl(m_impl->bridgeUrl);
}

OperationResult AutomationEventClient::connectToBridge(int timeoutMs)
{
    if (m_impl->streamClient.connectToBridge(timeoutMs)) {
        m_impl->error.clear();
        return OperationResult{};
    }

    m_impl->error = m_impl->streamClient.errorString();
    return operationTransportFailure(m_impl->error);
}

void AutomationEventClient::disconnectFromBridge()
{
    m_impl->streamClient.disconnectFromBridge();
}

bool AutomationEventClient::isConnected() const
{
    return m_impl->streamClient.isConnected();
}

QString AutomationEventClient::errorString() const
{
    return m_impl->error;
}

Result<QVector<EventKind>> AutomationEventClient::supportedEvents(int timeoutMs)
{
    const BridgeCallResult callResult = m_impl->streamClient.call(QStringLiteral("list_event_types"), QJsonObject{}, timeoutMs);
    if (!callResult.transportOk) {
        return transportFailure<QVector<EventKind>>(callResult.transportError);
    }
    if (!callResult.response.value(QStringLiteral("ok")).toBool()) {
        return bridgeFailure<QVector<EventKind>>(callResult.response);
    }

    Result<QVector<EventKind>> result;
    const QJsonArray array = callResult.response.value(QStringLiteral("result")).toObject().value(QStringLiteral("events")).toArray();
    result.value.reserve(array.size());
    for (const QJsonValue& value : array) {
        result.value.append(detail::eventKindFromString(value.toString()));
    }
    return result;
}

Result<SubscriptionHandle> AutomationEventClient::subscribe(const EventSubscriptionSpec& spec, int timeoutMs)
{
    QStringList events;
    events.reserve(spec.events.size());
    for (EventKind event : spec.events) {
        const QString name = toString(event);
        if (!name.isEmpty() && name != QStringLiteral("unknown")) {
            events.append(name);
        }
    }

    const BridgeCallResult callResult = m_impl->streamClient.subscribe(events, detail::encodeSelectors(spec.selectors),
                                                                       spec.debounceMs, timeoutMs);
    if (!callResult.transportOk) {
        return transportFailure<SubscriptionHandle>(callResult.transportError);
    }
    if (!callResult.response.value(QStringLiteral("ok")).toBool()) {
        return bridgeFailure<SubscriptionHandle>(callResult.response);
    }

    Result<SubscriptionHandle> result;
    result.value.id = callResult.response.value(QStringLiteral("result")).toObject()
                          .value(QStringLiteral("subscriptionId")).toString();
    return result;
}

OperationResult AutomationEventClient::unsubscribe(const SubscriptionHandle& handle, int timeoutMs)
{
    const BridgeCallResult callResult = m_impl->streamClient.unsubscribe(handle.id, timeoutMs);
    if (!callResult.transportOk) {
        return operationTransportFailure(callResult.transportError);
    }
    if (!callResult.response.value(QStringLiteral("ok")).toBool()) {
        return operationBridgeFailure(callResult.response);
    }

    return OperationResult{};
}

void AutomationEventClient::setConnectedHandler(std::function<void()> handler)
{
    m_impl->connectedHandler = std::move(handler);
}

void AutomationEventClient::setDisconnectedHandler(std::function<void()> handler)
{
    m_impl->disconnectedHandler = std::move(handler);
}

void AutomationEventClient::setTransportErrorHandler(std::function<void(const QString&)> handler)
{
    m_impl->transportErrorHandler = std::move(handler);
}

void AutomationEventClient::setTabChangedHandler(std::function<void(const TabChangedEvent&)> handler)
{
    m_impl->tabChangedHandler = std::move(handler);
}

void AutomationEventClient::setActivePageChangedHandler(std::function<void(const ActivePageChangedEvent&)> handler)
{
    m_impl->activePageChangedHandler = std::move(handler);
}

void AutomationEventClient::setWindowFocusChangedHandler(std::function<void(const WindowFocusChangedEvent&)> handler)
{
    m_impl->windowFocusChangedHandler = std::move(handler);
}

void AutomationEventClient::setFocusWidgetChangedHandler(std::function<void(const FocusWidgetChangedEvent&)> handler)
{
    m_impl->focusWidgetChangedHandler = std::move(handler);
}

void AutomationEventClient::setModalDialogChangedHandler(std::function<void(const ModalDialogChangedEvent&)> handler)
{
    m_impl->modalDialogChangedHandler = std::move(handler);
}

} // namespace qtautotest
