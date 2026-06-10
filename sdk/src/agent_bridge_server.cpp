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

    const QStringList& supportedEventTypes()
    {
        static const QStringList kTypes{
            QStringLiteral("tab_changed"),
            QStringLiteral("active_page_changed"),
            QStringLiteral("window_focus_changed"),
            QStringLiteral("focus_widget_changed"),
            QStringLiteral("modal_dialog_changed"),
        };
        return kTypes;
    }

    const QJsonArray& supportedEventTypesJson()
    {
        static const QJsonArray kJson = QJsonArray::fromStringList(supportedEventTypes());
        return kJson;
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
    : QObject(parent), m_server(new QWebSocketServer(QStringLiteral("QtAutoTest Agent Bridge"),
                                                     QWebSocketServer::NonSecureMode, this)),
      m_eventMonitor(new UiEventMonitor(this))
{
    connect(m_server, &QWebSocketServer::newConnection, this, &AgentBridgeServer::handleNewConnection);
    connect(m_eventMonitor, &UiEventMonitor::eventObserved,
            this, &AgentBridgeServer::handleUiEvent);

    initCommandHandlers();
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

void AgentBridgeServer::initCommandHandlers()
{
    // ---- 无参数只读命令 ----
    m_commandHandlers[QStringLiteral("ping")] = [this](QWebSocket *, const QJsonObject &, const QJsonValue &id) {
        return successResponse(id, QJsonObject{{"message", QStringLiteral("pong")}});
    };

    m_commandHandlers[QStringLiteral("list_commands")] = [this](QWebSocket *, const QJsonObject &, const QJsonValue &id) {
        // Qt 5.14 QJsonArray 不支持 reserve() 和 std::sort
        // （迭代器解引用为 QJsonValueRef，不可 std::swap）
        // 改用 QStringList 排序后转换
        QStringList commandNames;
        commandNames.reserve(m_commandHandlers.size());
        for (auto it = m_commandHandlers.constBegin(); it != m_commandHandlers.constEnd(); ++it) {
            commandNames.append(it.key());
        }
        commandNames.sort();
        QJsonArray commands;
        // reserve() not available in Qt 5.14 QJsonArray
        for (const QString& name : std::as_const(commandNames)) {
            commands.append(name);
        }
        return successResponse(id, QJsonObject{{QStringLiteral("commands"), commands}});
    };

    m_commandHandlers[QStringLiteral("list_event_types")] = [this](QWebSocket *, const QJsonObject &, const QJsonValue &id) {
        return successResponse(id, QJsonObject{{QStringLiteral("events"), supportedEventTypesJson()}});
    };

    m_commandHandlers[QStringLiteral("describe_ui")] = [this](QWebSocket *, const QJsonObject &, const QJsonValue &id) {
        return successResponse(id, WidgetIntrospection::describeUi());
    };

    m_commandHandlers[QStringLiteral("describe_snapshot")] = [this](QWebSocket *, const QJsonObject &, const QJsonValue &id) {
        return wrapResult(id, BridgeOperations::describeSnapshot());
    };

    m_commandHandlers[QStringLiteral("describe_active_page")] = [this](QWebSocket *, const QJsonObject &, const QJsonValue &id) {
        return wrapResult(id, BridgeOperations::describeActivePage());
    };

    m_commandHandlers[QStringLiteral("list_windows")] = [this](QWebSocket *, const QJsonObject &, const QJsonValue &id) {
        return wrapResult(id, BridgeOperations::listWindows());
    };

    // ---- 布尔参数只读命令 ----
    auto visibleOnlyHandler = [this](const QString &command, const QJsonObject &request, const QJsonValue &id) {
        const bool visibleOnly = request.value(QStringLiteral("visibleOnly")).toBool(false);
        if (command == QStringLiteral("describe_object_tree")) {
            return wrapResult(id, BridgeOperations::describeObjectTree(visibleOnly));
        }
        return wrapResult(id, BridgeOperations::describeLayoutTree(visibleOnly));
    };

    m_commandHandlers[QStringLiteral("describe_object_tree")] = [visibleOnlyHandler](QWebSocket *, const QJsonObject &req, const QJsonValue &id) {
        return visibleOnlyHandler(QStringLiteral("describe_object_tree"), req, id);
    };

    m_commandHandlers[QStringLiteral("describe_layout_tree")] = [visibleOnlyHandler](QWebSocket *, const QJsonObject &req, const QJsonValue &id) {
        return visibleOnlyHandler(QStringLiteral("describe_layout_tree"), req, id);
    };

    // ---- 带 selector 的只读命令 ----
    auto selectorOnlyHandler = [this](const QString &bridgeCommand, const QJsonObject &request, const QJsonValue &id) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();

        if (bridgeCommand == QStringLiteral("describe_subtree")) {
            const bool layoutTree = request.value(QStringLiteral("layoutTree")).toBool(false);
            const bool visibleOnly = request.value(QStringLiteral("visibleOnly")).toBool(false);
            return wrapResult(id, BridgeOperations::describeSubtree(selector, layoutTree, visibleOnly));
        }

        if (bridgeCommand == QStringLiteral("describe_style")) {
            const bool includeChildren = request.value(QStringLiteral("includeChildren")).toBool(false);
            QString errorCode;
            QString errorMessage;
            QJsonArray candidates;
            const QJsonObject result = WidgetIntrospection::describeStyle(
                selector, includeChildren, &errorCode, &errorMessage, &candidates);
            if (result.isEmpty()) {
                QJsonObject details;
                if (!candidates.isEmpty()) {
                    details.insert(QStringLiteral("candidates"), candidates);
                }
                return errorResponse(id, errorCode, errorMessage, details);
            }
            return successResponse(id, result);
        }

        if (bridgeCommand == QStringLiteral("focus_window")) {
            return wrapResult(id, BridgeOperations::focusWindow(selector));
        }

        if (bridgeCommand == QStringLiteral("find_widgets")) {
            return successResponse(id, QJsonObject{{QStringLiteral("matches"), WidgetIntrospection::findWidgets(selector)}});
        }

        if (bridgeCommand == QStringLiteral("capture_window")) {
            return wrapResult(id, UiActionExecutor::captureWindow(selector));
        }

        return errorResponse(id, QStringLiteral("unknown_command"), QString());
    };

    m_commandHandlers[QStringLiteral("describe_subtree")] = [selectorOnlyHandler](QWebSocket *, const QJsonObject &req, const QJsonValue &id) {
        return selectorOnlyHandler(QStringLiteral("describe_subtree"), req, id);
    };
    m_commandHandlers[QStringLiteral("describe_style")] = [selectorOnlyHandler](QWebSocket *, const QJsonObject &req, const QJsonValue &id) {
        return selectorOnlyHandler(QStringLiteral("describe_style"), req, id);
    };
    m_commandHandlers[QStringLiteral("focus_window")] = [selectorOnlyHandler](QWebSocket *, const QJsonObject &req, const QJsonValue &id) {
        return selectorOnlyHandler(QStringLiteral("focus_window"), req, id);
    };
    m_commandHandlers[QStringLiteral("find_widgets")] = [selectorOnlyHandler](QWebSocket *, const QJsonObject &req, const QJsonValue &id) {
        return selectorOnlyHandler(QStringLiteral("find_widgets"), req, id);
    };
    m_commandHandlers[QStringLiteral("capture_window")] = [selectorOnlyHandler](QWebSocket *, const QJsonObject &req, const QJsonValue &id) {
        return selectorOnlyHandler(QStringLiteral("capture_window"), req, id);
    };

    // ---- 动作命令（selector + 参数） ----
    m_commandHandlers[QStringLiteral("click")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::click(request.value(QStringLiteral("selector")).toObject()));
    };

    m_commandHandlers[QStringLiteral("set_text")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::setText(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("text")).toString()));
    };

    m_commandHandlers[QStringLiteral("press_key")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::pressKey(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("key")).toString(),
                              request.value(QStringLiteral("modifiers")).toString()));
    };

    m_commandHandlers[QStringLiteral("send_shortcut")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::sendShortcut(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("shortcut")).toString()));
    };

    m_commandHandlers[QStringLiteral("scroll")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::scroll(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("direction")).toString(),
                              request.value(QStringLiteral("amount")).toInt(kDefaultScrollAmount)));
    };

    m_commandHandlers[QStringLiteral("scroll_into_view")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::scrollIntoView(
                              request.value(QStringLiteral("selector")).toObject()));
    };

    m_commandHandlers[QStringLiteral("select_item")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::selectItem(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("options")).toObject()));
    };

    m_commandHandlers[QStringLiteral("toggle_check")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::toggleCheck(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("checked")).toBool()));
    };

    m_commandHandlers[QStringLiteral("choose_combo_option")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::chooseComboOption(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("text")).toString(),
                              request.value(QStringLiteral("index")).toInt(-1)));
    };

    m_commandHandlers[QStringLiteral("activate_tab")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::activateTab(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("text")).toString(),
                              request.value(QStringLiteral("index")).toInt(-1)));
    };

    m_commandHandlers[QStringLiteral("switch_stacked_page")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::switchStackedPage(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("index")).toInt(-1)));
    };

    m_commandHandlers[QStringLiteral("expand_tree_node")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::expandTreeNode(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("path")).toArray()));
    };

    m_commandHandlers[QStringLiteral("collapse_tree_node")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, UiActionExecutor::collapseTreeNode(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("path")).toArray()));
    };

    // ---- 订阅管理（需要 socket） ----
    m_commandHandlers[QStringLiteral("subscribe")] = [this](QWebSocket *socket, const QJsonObject &request, const QJsonValue &id) {
        const QJsonArray eventsArray = request.value(QStringLiteral("events")).toArray();
        if (eventsArray.isEmpty()) {
            return errorResponse(id, QStringLiteral("missing_events"),
                                 QStringLiteral("Subscribe requires at least one event name."));
        }

        QStringList events;
        QStringList invalidEvents;
        const QStringList supported = supportedEventTypes();
        for (const QJsonValue &value : eventsArray) {
            const QString eventName = value.toString();
            if (eventName.isEmpty()) continue;
            if (!supported.contains(eventName)) {
                invalidEvents.append(eventName);
                continue;
            }
            if (!events.contains(eventName)) {
                events.append(eventName);
            }
        }

        if (events.isEmpty()) {
            return errorResponse(id, QStringLiteral("missing_events"),
                                 QStringLiteral("Subscribe requires at least one valid event name."));
        }

        if (!invalidEvents.isEmpty()) {
            return errorResponse(id, QStringLiteral("unsupported_event"),
                                 QStringLiteral("Subscribe requested unsupported events."),
                                 QJsonObject{{QStringLiteral("events"), QJsonArray::fromStringList(invalidEvents)}});
        }

        const QJsonArray selectors = request.value(QStringLiteral("selectors")).toArray();
        for (const QJsonValue &selectorValue : selectors) {
            if (!selectorValue.isObject()) {
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
                                   {QStringLiteral("subscriptionId"), subscription.id},
                                   {QStringLiteral("events"), QJsonArray::fromStringList(subscription.events)},
                                   {QStringLiteral("debounceMs"), subscription.debounceMs},
                               });
    };

    m_commandHandlers[QStringLiteral("unsubscribe")] = [this](QWebSocket *socket, const QJsonObject &request, const QJsonValue &id) {
        const QString subscriptionId = request.value(QStringLiteral("subscriptionId")).toString();
        if (subscriptionId.isEmpty()) {
            return errorResponse(id, QStringLiteral("missing_subscription_id"),
                                 QStringLiteral("Unsubscribe requires subscriptionId."));
        }

        for (int i = 0; i < m_subscriptions.size(); ++i) {
            const EventSubscription &subscription = m_subscriptions.at(i);
            if (subscription.id == subscriptionId && subscription.socket == socket) {
                m_subscriptions.removeAt(i);
                return successResponse(id, QJsonObject{{QStringLiteral("subscriptionId"), subscriptionId}});
            }
        }

        return errorResponse(id, QStringLiteral("subscription_not_found"),
                             QStringLiteral("Subscription was not found for this connection."));
    };

    // ---- 断言和等待 ----
    m_commandHandlers[QStringLiteral("assert_widget")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, BridgeOperations::assertWidget(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("assertions")).toObject()));
    };

    m_commandHandlers[QStringLiteral("wait_for_widget")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, BridgeOperations::waitForWidget(
                              request.value(QStringLiteral("selector")).toObject(),
                              request.value(QStringLiteral("assertions")).toObject(),
                              request.value(QStringLiteral("timeoutMs")).toInt(kDefaultTimeoutMs),
                              request.value(QStringLiteral("pollIntervalMs")).toInt(kDefaultPollIntervalMs)));
    };

    m_commandHandlers[QStringLiteral("wait_for_log")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        return wrapResult(id, BridgeOperations::waitForLog(
                              request.value(QStringLiteral("textContains")).toString(),
                              request.value(QStringLiteral("regex")).toString(),
                              request.value(QStringLiteral("timeoutMs")).toInt(kDefaultTimeoutMs),
                              request.value(QStringLiteral("pollIntervalMs")).toInt(kDefaultPollIntervalMs),
                              request.value(QStringLiteral("limit")).toInt(kDefaultWaitLogLimit)));
    };

    // ---- 日志 ----
    m_commandHandlers[QStringLiteral("get_logs")] = [this](QWebSocket *, const QJsonObject &request, const QJsonValue &id) {
        const int limit = request.value(QStringLiteral("limit")).toInt(kDefaultLogLimit);
        return successResponse(id, QJsonObject{{QStringLiteral("entries"), AppLogSink::instance().recentEntries(limit)}});
    };
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

    const auto it = m_commandHandlers.constFind(command);
    if (it != m_commandHandlers.constEnd())
    {
        return it.value()(socket, request, id);
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

QJsonObject AgentBridgeServer::wrapResult(const QJsonValue &id, const QJsonObject &result) const
{
    QJsonObject details = result;
    details.remove(QStringLiteral("ok"));
    if (!result.value(QStringLiteral("ok")).toBool())
    {
        return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                             result.value(QStringLiteral("message")).toString(), details);
    }
    return successResponse(id, details);
}
