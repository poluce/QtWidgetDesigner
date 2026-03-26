#include "agent_bridge_server.h"

#include "app_log_sink.h"
#include "bridge_operations.h"
#include "ui_event_monitor.h"
#include "ui_action_executor.h"
#include "widget_introspection.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QDateTime>
#include <QWebSocket>
#include <QWebSocketServer>

namespace
{

    QStringList supportedEventTypes()
    {
        return QStringList{
            QStringLiteral("tab_changed"),
            QStringLiteral("active_page_changed"),
            QStringLiteral("window_focus_changed"),
            QStringLiteral("focus_widget_changed"),
            QStringLiteral("modal_dialog_changed"),
        };
    }

    QJsonArray supportedEventTypesJson()
    {
        return QJsonArray::fromStringList(supportedEventTypes());
    }

    QString payloadFingerprint(const QString &eventName, const QJsonObject &payload)
    {
        return eventName + QStringLiteral(":") +
               QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }

    constexpr int kDefaultDebounceMs = 50;
    constexpr int kDefaultLogLimit = 50;
    constexpr int kDefaultWaitLogLimit = 200;
    constexpr int kDefaultTimeoutMs = 3000;
    constexpr int kDefaultPollIntervalMs = 100;
    constexpr int kDefaultScrollAmount = 120;

} // namespace

AgentBridgeServer::AgentBridgeServer(QObject *parent)
    : QObject(parent), m_server(new QWebSocketServer(QStringLiteral("QtAgentAutotest Agent Bridge"),
                                                     QWebSocketServer::NonSecureMode, this)),
      m_eventMonitor(new UiEventMonitor(this))
{
    connect(m_server, &QWebSocketServer::newConnection, this, &AgentBridgeServer::handleNewConnection);
    connect(m_eventMonitor, &UiEventMonitor::eventObserved,
            this, &AgentBridgeServer::handleUiEvent);
}

AgentBridgeServer::~AgentBridgeServer()
{
    for (QWebSocket *client : std::as_const(m_clients))
    {
        if (client != nullptr)
        {
            disconnect(client, nullptr, this, nullptr);
            client->close();
            client->deleteLater();
        }
    }
    m_clients.clear();
    m_subscriptions.clear();
}

bool AgentBridgeServer::start(quint16 portNumber)
{
    return m_server->listen(QHostAddress::LocalHost, portNumber);
}

quint16 AgentBridgeServer::port() const
{
    return m_server->serverPort();
}

QString AgentBridgeServer::errorString() const
{
    return m_server->errorString();
}

void AgentBridgeServer::handleNewConnection()
{
    QWebSocket *socket = m_server->nextPendingConnection();
    m_clients.append(socket);
    m_nextSequenceBySocket.insert(socket, 1);

    connect(socket, &QWebSocket::textMessageReceived, this,
            [this, socket](const QString &message)
            { handleTextMessage(socket, message); });
    connect(socket, &QWebSocket::disconnected, this,
            [this, socket]()
            { handleDisconnected(socket); });
}

void AgentBridgeServer::handleTextMessage(QWebSocket *socket, const QString &message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        const QJsonObject response = errorResponse(
            QJsonValue(),
            QStringLiteral("invalid_json"),
            QStringLiteral("Request must be a valid JSON object."));
        socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
        return;
    }

    const QJsonObject response = dispatch(socket, document.object());
    socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
}

void AgentBridgeServer::handleDisconnected(QWebSocket *socket)
{
    m_clients.removeAll(socket);
    m_nextSequenceBySocket.remove(socket);
    for (int i = m_subscriptions.size() - 1; i >= 0; --i)
    {
        if (m_subscriptions.at(i).socket == socket)
        {
            m_subscriptions.removeAt(i);
        }
    }
    socket->deleteLater();
}

void AgentBridgeServer::handleUiEvent(const QString &eventName, const QJsonObject &payload, const QStringList &relatedRefs)
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    for (int i = m_subscriptions.size() - 1; i >= 0; --i)
    {
        EventSubscription &subscription = m_subscriptions[i];
        QWebSocket *socket = subscription.socket.data();
        if (socket == nullptr)
        {
            m_subscriptions.removeAt(i);
            continue;
        }

        if (!subscriptionMatches(subscription, eventName, relatedRefs))
        {
            continue;
        }

        const QString fingerprint = payloadFingerprint(eventName, payload);
        const qint64 lastSentMs = subscription.lastSentAtByFingerprint.value(fingerprint, -1);
        if (subscription.debounceMs > 0 && lastSentMs >= 0 &&
            nowMs - lastSentMs < subscription.debounceMs)
        {
            continue;
        }

        subscription.lastSentAtByFingerprint.insert(fingerprint, nowMs);
        sendEventFrame(socket, subscription.id, eventName, payload);
    }
}

bool AgentBridgeServer::subscriptionMatches(const EventSubscription &subscription, const QString &eventName,
                                            const QStringList &relatedRefs) const
{
    if (!subscription.events.contains(eventName))
    {
        return false;
    }

    if (subscription.selectors.isEmpty())
    {
        return true;
    }

    if (relatedRefs.isEmpty())
    {
        return false;
    }

    for (const QJsonValue &selectorValue : subscription.selectors)
    {
        if (!selectorValue.isObject())
        {
            continue;
        }

        const QJsonArray matches = WidgetIntrospection::findWidgets(selectorValue.toObject());
        for (const QJsonValue &matchValue : matches)
        {
            const QString ref = matchValue.toObject().value(QStringLiteral("ref")).toString();
            if (!ref.isEmpty() && relatedRefs.contains(ref))
            {
                return true;
            }
        }
    }

    return false;
}

void AgentBridgeServer::sendEventFrame(QWebSocket *socket, const QString &subscriptionId,
                                       const QString &eventName, const QJsonObject &payload)
{
    const quint64 sequence = m_nextSequenceBySocket.value(socket, 1);
    m_nextSequenceBySocket.insert(socket, sequence + 1);

    const QJsonObject eventFrame{
        {"type", QStringLiteral("event")},
        {"event", eventName},
        {"subscriptionId", subscriptionId},
        {"sequence", static_cast<qint64>(sequence)},
        {"payload", payload},
    };

    socket->sendTextMessage(QString::fromUtf8(QJsonDocument(eventFrame).toJson(QJsonDocument::Compact)));
}

QJsonObject AgentBridgeServer::dispatch(QWebSocket *socket, const QJsonObject &request)
{
    const QJsonValue id = request.value(QStringLiteral("id"));
    const QString command = request.value(QStringLiteral("command")).toString();

    if (command.isEmpty())
    {
        return errorResponse(id, QStringLiteral("missing_command"), QStringLiteral("Request is missing command."));
    }

    qInfo().noquote() << QStringLiteral("[桥接] 命令=%1 载荷=%2")
                             .arg(command, QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));

    if (command == QStringLiteral("ping"))
    {
        return successResponse(id, QJsonObject{{"message", QStringLiteral("pong")}});
    }

    if (command == QStringLiteral("list_commands"))
    {
        return successResponse(id, QJsonObject{
                                       {"commands", QJsonArray{
                                                        QStringLiteral("ping"),
                                                        QStringLiteral("list_commands"),
                                                        QStringLiteral("list_event_types"),
                                                        QStringLiteral("subscribe"),
                                                        QStringLiteral("unsubscribe"),
                                                        QStringLiteral("describe_ui"),
                                                        QStringLiteral("describe_snapshot"),
                                                        QStringLiteral("describe_object_tree"),
                                                        QStringLiteral("describe_layout_tree"),
                                                        QStringLiteral("describe_subtree"),
                                                        QStringLiteral("describe_style"),
                                                        QStringLiteral("describe_active_page"),
                                                        QStringLiteral("list_windows"),
                                                        QStringLiteral("focus_window"),
                                                        QStringLiteral("find_widgets"),
                                                        QStringLiteral("click"),
                                                        QStringLiteral("set_text"),
                                                        QStringLiteral("press_key"),
                                                        QStringLiteral("send_shortcut"),
                                                        QStringLiteral("scroll"),
                                                        QStringLiteral("scroll_into_view"),
                                                        QStringLiteral("select_item"),
                                                        QStringLiteral("toggle_check"),
                                                        QStringLiteral("choose_combo_option"),
                                                        QStringLiteral("activate_tab"),
                                                        QStringLiteral("switch_stacked_page"),
                                                        QStringLiteral("expand_tree_node"),
                                                        QStringLiteral("collapse_tree_node"),
                                                        QStringLiteral("assert_widget"),
                                                        QStringLiteral("wait_for_widget"),
                                                        QStringLiteral("wait_for_log"),
                                                        QStringLiteral("get_logs"),
                                                        QStringLiteral("capture_window"),
                                                    }},
                                   });
    }

    if (command == QStringLiteral("list_event_types"))
    {
        return successResponse(id, QJsonObject{
                                       {"events", supportedEventTypesJson()},
                                   });
    }

    if (command == QStringLiteral("subscribe"))
    {
        const QJsonArray eventsArray = request.value(QStringLiteral("events")).toArray();
        if (eventsArray.isEmpty())
        {
            return errorResponse(id, QStringLiteral("missing_events"),
                                 QStringLiteral("Subscribe requires at least one event name."));
        }

        QStringList events;
        QStringList invalidEvents;
        const QStringList supported = supportedEventTypes();
        for (const QJsonValue &value : eventsArray)
        {
            const QString eventName = value.toString();
            if (eventName.isEmpty())
            {
                continue;
            }
            if (!supported.contains(eventName))
            {
                invalidEvents.append(eventName);
                continue;
            }
            if (!events.contains(eventName))
            {
                events.append(eventName);
            }
        }

        if (events.isEmpty())
        {
            return errorResponse(id, QStringLiteral("missing_events"),
                                 QStringLiteral("Subscribe requires at least one valid event name."));
        }

        if (!invalidEvents.isEmpty())
        {
            return errorResponse(id, QStringLiteral("unsupported_event"),
                                 QStringLiteral("Subscribe requested unsupported events."),
                                 QJsonObject{{"events", QJsonArray::fromStringList(invalidEvents)}});
        }

        const QJsonArray selectors = request.value(QStringLiteral("selectors")).toArray();
        for (const QJsonValue &selectorValue : selectors)
        {
            if (!selectorValue.isObject())
            {
                return errorResponse(id, QStringLiteral("invalid_selector"),
                                     QStringLiteral("Each selector must be a JSON object."));
            }
        }

        EventSubscription subscription;
        subscription.id = QStringLiteral("sub-%1").arg(m_nextSubscriptionId++);
        subscription.socket = socket;
        subscription.events = events;
        subscription.selectors = selectors;
        subscription.debounceMs = qMax(0, request.value(QStringLiteral("debounceMs")).toInt(kDefaultDebounceMs));
        m_subscriptions.append(subscription);

        return successResponse(id, QJsonObject{
                                       {"subscriptionId", subscription.id},
                                       {"events", QJsonArray::fromStringList(subscription.events)},
                                       {"debounceMs", subscription.debounceMs},
                                   });
    }

    if (command == QStringLiteral("unsubscribe"))
    {
        const QString subscriptionId = request.value(QStringLiteral("subscriptionId")).toString();
        if (subscriptionId.isEmpty())
        {
            return errorResponse(id, QStringLiteral("missing_subscription_id"),
                                 QStringLiteral("Unsubscribe requires subscriptionId."));
        }

        for (int i = 0; i < m_subscriptions.size(); ++i)
        {
            const EventSubscription &subscription = m_subscriptions.at(i);
            if (subscription.id == subscriptionId && subscription.socket == socket)
            {
                m_subscriptions.removeAt(i);
                return successResponse(id, QJsonObject{{"subscriptionId", subscriptionId}});
            }
        }

        return errorResponse(id, QStringLiteral("subscription_not_found"),
                             QStringLiteral("Subscription was not found for this connection."));
    }

    if (command == QStringLiteral("describe_ui"))
    {
        return successResponse(id, WidgetIntrospection::describeUi());
    }

    // Helper: wrap a command result into success/error response
    auto wrapResult = [&](const QJsonObject &result) -> QJsonObject
    {
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        if (!result.value(QStringLiteral("ok")).toBool())
        {
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        return successResponse(id, details);
    };

    if (command == QStringLiteral("describe_snapshot"))
    {
        return wrapResult(BridgeOperations::describeSnapshot());
    }

    if (command == QStringLiteral("describe_object_tree"))
    {
        const bool visibleOnly = request.value(QStringLiteral("visibleOnly")).toBool(false);
        return wrapResult(BridgeOperations::describeObjectTree(visibleOnly));
    }

    if (command == QStringLiteral("describe_layout_tree"))
    {
        const bool visibleOnly = request.value(QStringLiteral("visibleOnly")).toBool(false);
        return wrapResult(BridgeOperations::describeLayoutTree(visibleOnly));
    }

    if (command == QStringLiteral("describe_subtree"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const bool layoutTree = request.value(QStringLiteral("layoutTree")).toBool(false);
        const bool visibleOnly = request.value(QStringLiteral("visibleOnly")).toBool(false);
        return wrapResult(BridgeOperations::describeSubtree(selector, layoutTree, visibleOnly));
    }

    if (command == QStringLiteral("describe_style"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const bool includeChildren = request.value(QStringLiteral("includeChildren")).toBool(false);
        QString errorCode;
        QString errorMessage;
        QJsonArray candidates;
        const QJsonObject result = WidgetIntrospection::describeStyle(
            selector, includeChildren, &errorCode, &errorMessage, &candidates);
        if (result.isEmpty())
        {
            QJsonObject details;
            if (!candidates.isEmpty())
            {
                details.insert(QStringLiteral("candidates"), candidates);
            }
            return errorResponse(id, errorCode, errorMessage, details);
        }
        return successResponse(id, result);
    }

    if (command == QStringLiteral("describe_active_page"))
    {
        return wrapResult(BridgeOperations::describeActivePage());
    }

    if (command == QStringLiteral("list_windows"))
    {
        return wrapResult(BridgeOperations::listWindows());
    }

    if (command == QStringLiteral("focus_window"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        return wrapResult(BridgeOperations::focusWindow(selector));
    }

    if (command == QStringLiteral("find_widgets"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        return successResponse(id, QJsonObject{{"matches", WidgetIntrospection::findWidgets(selector)}});
    }

    if (command == QStringLiteral("click"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        return wrapResult(UiActionExecutor::click(selector));
    }

    if (command == QStringLiteral("set_text"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString text = request.value(QStringLiteral("text")).toString();
        return wrapResult(UiActionExecutor::setText(selector, text));
    }

    if (command == QStringLiteral("press_key"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString key = request.value(QStringLiteral("key")).toString();
        const QString modifiers = request.value(QStringLiteral("modifiers")).toString();
        return wrapResult(UiActionExecutor::pressKey(selector, key, modifiers));
    }

    if (command == QStringLiteral("send_shortcut"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString shortcut = request.value(QStringLiteral("shortcut")).toString();
        return wrapResult(UiActionExecutor::sendShortcut(selector, shortcut));
    }

    if (command == QStringLiteral("scroll"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString direction = request.value(QStringLiteral("direction")).toString();
        const int amount = request.value(QStringLiteral("amount")).toInt(kDefaultScrollAmount);
        return wrapResult(UiActionExecutor::scroll(selector, direction, amount));
    }

    if (command == QStringLiteral("scroll_into_view"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        return wrapResult(UiActionExecutor::scrollIntoView(selector));
    }

    if (command == QStringLiteral("select_item"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject options = request.value(QStringLiteral("options")).toObject();
        return wrapResult(UiActionExecutor::selectItem(selector, options));
    }

    if (command == QStringLiteral("toggle_check"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const bool checked = request.value(QStringLiteral("checked")).toBool();
        return wrapResult(UiActionExecutor::toggleCheck(selector, checked));
    }

    if (command == QStringLiteral("choose_combo_option"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString text = request.value(QStringLiteral("text")).toString();
        const int index = request.value(QStringLiteral("index")).toInt(-1);
        return wrapResult(UiActionExecutor::chooseComboOption(selector, text, index));
    }

    if (command == QStringLiteral("activate_tab"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString text = request.value(QStringLiteral("text")).toString();
        const int index = request.value(QStringLiteral("index")).toInt(-1);
        return wrapResult(UiActionExecutor::activateTab(selector, text, index));
    }

    if (command == QStringLiteral("switch_stacked_page"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const int index = request.value(QStringLiteral("index")).toInt(-1);
        return wrapResult(UiActionExecutor::switchStackedPage(selector, index));
    }

    if (command == QStringLiteral("expand_tree_node"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonArray path = request.value(QStringLiteral("path")).toArray();
        return wrapResult(UiActionExecutor::expandTreeNode(selector, path));
    }

    if (command == QStringLiteral("collapse_tree_node"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonArray path = request.value(QStringLiteral("path")).toArray();
        return wrapResult(UiActionExecutor::collapseTreeNode(selector, path));
    }

    if (command == QStringLiteral("get_logs"))
    {
        const int limit = request.value(QStringLiteral("limit")).toInt(kDefaultLogLimit);
        return successResponse(id, QJsonObject{{"entries", AppLogSink::instance().recentEntries(limit)}});
    }

    if (command == QStringLiteral("assert_widget"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject assertions = request.value(QStringLiteral("assertions")).toObject();
        return wrapResult(BridgeOperations::assertWidget(selector, assertions));
    }

    if (command == QStringLiteral("wait_for_widget"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject assertions = request.value(QStringLiteral("assertions")).toObject();
        const int timeoutMs = request.value(QStringLiteral("timeoutMs")).toInt(kDefaultTimeoutMs);
        const int pollIntervalMs = request.value(QStringLiteral("pollIntervalMs")).toInt(kDefaultPollIntervalMs);
        return wrapResult(BridgeOperations::waitForWidget(selector, assertions, timeoutMs, pollIntervalMs));
    }

    if (command == QStringLiteral("wait_for_log"))
    {
        const QString textContains = request.value(QStringLiteral("textContains")).toString();
        const QString regex = request.value(QStringLiteral("regex")).toString();
        const int timeoutMs = request.value(QStringLiteral("timeoutMs")).toInt(kDefaultTimeoutMs);
        const int pollIntervalMs = request.value(QStringLiteral("pollIntervalMs")).toInt(kDefaultPollIntervalMs);
        const int limit = request.value(QStringLiteral("limit")).toInt(kDefaultWaitLogLimit);
        return wrapResult(BridgeOperations::waitForLog(textContains, regex, timeoutMs, pollIntervalMs, limit));
    }

    if (command == QStringLiteral("capture_window"))
    {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        return wrapResult(UiActionExecutor::captureWindow(selector));
    }

    return errorResponse(id, QStringLiteral("unknown_command"),
                         QStringLiteral("Unsupported command: %1").arg(command));
}

QJsonObject AgentBridgeServer::successResponse(const QJsonValue &id, const QJsonObject &result) const
{
    QJsonObject response{
        {"ok", true},
        {"result", result},
    };

    if (!id.isUndefined())
    {
        response.insert(QStringLiteral("id"), id);
    }

    return response;
}

QJsonObject AgentBridgeServer::errorResponse(const QJsonValue &id, const QString &code, const QString &message,
                                             const QJsonObject &details) const
{
    QJsonObject error{
        {"code", code},
        {"message", message},
    };

    for (auto it = details.begin(); it != details.end(); ++it)
    {
        if (it.key() == QStringLiteral("code") || it.key() == QStringLiteral("message"))
        {
            continue;
        }
        error.insert(it.key(), it.value());
    }

    QJsonObject response{
        {"ok", false},
        {"error", error},
    };

    if (!id.isUndefined())
    {
        response.insert(QStringLiteral("id"), id);
    }

    return response;
}
