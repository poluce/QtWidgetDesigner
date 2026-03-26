#include "agent_bridge_server.h"

#include "app_log_sink.h"
#include "bridge_operations.h"
#include "ui_action_executor.h"
#include "widget_introspection.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QWebSocket>
#include <QWebSocketServer>

AgentBridgeServer::AgentBridgeServer(QObject* parent)
    : QObject(parent)
    , m_server(new QWebSocketServer(QStringLiteral("QtAgentAutotest Agent Bridge"),
                                    QWebSocketServer::NonSecureMode, this))
{
    connect(m_server, &QWebSocketServer::newConnection, this, &AgentBridgeServer::handleNewConnection);
}

AgentBridgeServer::~AgentBridgeServer()
{
    for (QWebSocket* client : std::as_const(m_clients)) {
        if (client != nullptr) {
            client->close();
        }
    }
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
    QWebSocket* socket = m_server->nextPendingConnection();
    m_clients.append(socket);

    connect(socket, &QWebSocket::textMessageReceived, this,
            [this, socket](const QString& message) { handleTextMessage(socket, message); });
    connect(socket, &QWebSocket::disconnected, this,
            [this, socket]() { handleDisconnected(socket); });
}

void AgentBridgeServer::handleTextMessage(QWebSocket* socket, const QString& message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        const QJsonObject response = errorResponse(
            QJsonValue(),
            QStringLiteral("invalid_json"),
            QStringLiteral("Request must be a valid JSON object."));
        socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
        return;
    }

    const QJsonObject response = dispatch(document.object());
    socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
}

void AgentBridgeServer::handleDisconnected(QWebSocket* socket)
{
    m_clients.removeAll(socket);
    socket->deleteLater();
}

QJsonObject AgentBridgeServer::dispatch(const QJsonObject& request) const
{
    const QJsonValue id = request.value(QStringLiteral("id"));
    const QString command = request.value(QStringLiteral("command")).toString();

    if (command.isEmpty()) {
        return errorResponse(id, QStringLiteral("missing_command"), QStringLiteral("Request is missing command."));
    }

    qInfo().noquote() << QStringLiteral("[桥接] 命令=%1 载荷=%2")
                             .arg(command, QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));

    if (command == QStringLiteral("ping")) {
        return successResponse(id, QJsonObject{{"message", QStringLiteral("pong")}});
    }

    if (command == QStringLiteral("list_commands")) {
        return successResponse(id, QJsonObject{
                                       {"commands", QJsonArray{
                                                        QStringLiteral("ping"),
                                                        QStringLiteral("list_commands"),
                                                        QStringLiteral("describe_ui"),
                                                        QStringLiteral("describe_snapshot"),
                                                        QStringLiteral("describe_object_tree"),
                                                        QStringLiteral("describe_layout_tree"),
                                                        QStringLiteral("describe_subtree"),
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

    if (command == QStringLiteral("describe_ui")) {
        return successResponse(id, WidgetIntrospection::describeUi());
    }

    if (command == QStringLiteral("describe_snapshot")) {
        const QJsonObject result = BridgeOperations::describeSnapshot();
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("describe_object_tree")) {
        const bool visibleOnly = request.value(QStringLiteral("visibleOnly")).toBool(false);
        const QJsonObject result = BridgeOperations::describeObjectTree(visibleOnly);
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("describe_layout_tree")) {
        const bool visibleOnly = request.value(QStringLiteral("visibleOnly")).toBool(false);
        const QJsonObject result = BridgeOperations::describeLayoutTree(visibleOnly);
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("describe_subtree")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const bool layoutTree = request.value(QStringLiteral("layoutTree")).toBool(false);
        const bool visibleOnly = request.value(QStringLiteral("visibleOnly")).toBool(false);
        const QJsonObject result = BridgeOperations::describeSubtree(selector, layoutTree, visibleOnly);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("describe_active_page")) {
        const QJsonObject result = BridgeOperations::describeActivePage();
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("list_windows")) {
        const QJsonObject result = BridgeOperations::listWindows();
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("focus_window")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject result = BridgeOperations::focusWindow(selector);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("find_widgets")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        return successResponse(id, QJsonObject{{"matches", WidgetIntrospection::findWidgets(selector)}});
    }

    if (command == QStringLiteral("click")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject result = UiActionExecutor::click(selector);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("set_text")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString text = request.value(QStringLiteral("text")).toString();
        const QJsonObject result = UiActionExecutor::setText(selector, text);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("press_key")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString key = request.value(QStringLiteral("key")).toString();
        const QString modifiers = request.value(QStringLiteral("modifiers")).toString();
        const QJsonObject result = UiActionExecutor::pressKey(selector, key, modifiers);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("send_shortcut")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString shortcut = request.value(QStringLiteral("shortcut")).toString();
        const QJsonObject result = UiActionExecutor::sendShortcut(selector, shortcut);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("scroll")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString direction = request.value(QStringLiteral("direction")).toString();
        const int amount = request.value(QStringLiteral("amount")).toInt(120);
        const QJsonObject result = UiActionExecutor::scroll(selector, direction, amount);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("scroll_into_view")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject result = UiActionExecutor::scrollIntoView(selector);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("select_item")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject options = request.value(QStringLiteral("options")).toObject();
        const QJsonObject result = UiActionExecutor::selectItem(selector, options);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("toggle_check")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const bool checked = request.value(QStringLiteral("checked")).toBool();
        const QJsonObject result = UiActionExecutor::toggleCheck(selector, checked);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("choose_combo_option")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString text = request.value(QStringLiteral("text")).toString();
        const int index = request.value(QStringLiteral("index")).toInt(-1);
        const QJsonObject result = UiActionExecutor::chooseComboOption(selector, text, index);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("activate_tab")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QString text = request.value(QStringLiteral("text")).toString();
        const int index = request.value(QStringLiteral("index")).toInt(-1);
        const QJsonObject result = UiActionExecutor::activateTab(selector, text, index);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("switch_stacked_page")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const int index = request.value(QStringLiteral("index")).toInt(-1);
        const QJsonObject result = UiActionExecutor::switchStackedPage(selector, index);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("expand_tree_node")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonArray path = request.value(QStringLiteral("path")).toArray();
        const QJsonObject result = UiActionExecutor::expandTreeNode(selector, path);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("collapse_tree_node")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonArray path = request.value(QStringLiteral("path")).toArray();
        const QJsonObject result = UiActionExecutor::collapseTreeNode(selector, path);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("get_logs")) {
        const int limit = request.value(QStringLiteral("limit")).toInt(50);
        return successResponse(id, QJsonObject{{"entries", AppLogSink::instance().recentEntries(limit)}});
    }

    if (command == QStringLiteral("assert_widget")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject assertions = request.value(QStringLiteral("assertions")).toObject();
        const QJsonObject result = BridgeOperations::assertWidget(selector, assertions);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("wait_for_widget")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject assertions = request.value(QStringLiteral("assertions")).toObject();
        const int timeoutMs = request.value(QStringLiteral("timeoutMs")).toInt(3000);
        const int pollIntervalMs = request.value(QStringLiteral("pollIntervalMs")).toInt(100);
        const QJsonObject result = BridgeOperations::waitForWidget(selector, assertions, timeoutMs, pollIntervalMs);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("wait_for_log")) {
        const QString textContains = request.value(QStringLiteral("textContains")).toString();
        const QString regex = request.value(QStringLiteral("regex")).toString();
        const int timeoutMs = request.value(QStringLiteral("timeoutMs")).toInt(3000);
        const int pollIntervalMs = request.value(QStringLiteral("pollIntervalMs")).toInt(100);
        const int limit = request.value(QStringLiteral("limit")).toInt(200);
        const QJsonObject result =
            BridgeOperations::waitForLog(textContains, regex, timeoutMs, pollIntervalMs, limit);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    if (command == QStringLiteral("capture_window")) {
        const QJsonObject selector = request.value(QStringLiteral("selector")).toObject();
        const QJsonObject result = UiActionExecutor::captureWindow(selector);
        if (!result.value(QStringLiteral("ok")).toBool()) {
            QJsonObject details = result;
            details.remove(QStringLiteral("ok"));
            return errorResponse(id, result.value(QStringLiteral("code")).toString(),
                                 result.value(QStringLiteral("message")).toString(), details);
        }
        QJsonObject details = result;
        details.remove(QStringLiteral("ok"));
        return successResponse(id, details);
    }

    return errorResponse(id, QStringLiteral("unknown_command"),
                         QStringLiteral("Unsupported command: %1").arg(command));
}

QJsonObject AgentBridgeServer::successResponse(const QJsonValue& id, const QJsonObject& result) const
{
    QJsonObject response{
        {"ok", true},
        {"result", result},
    };

    if (!id.isUndefined()) {
        response.insert(QStringLiteral("id"), id);
    }

    return response;
}

QJsonObject AgentBridgeServer::errorResponse(const QJsonValue& id, const QString& code, const QString& message,
                                             const QJsonObject& details) const
{
    QJsonObject error{
        {"code", code},
        {"message", message},
    };

    for (auto it = details.begin(); it != details.end(); ++it) {
        if (it.key() == QStringLiteral("code") || it.key() == QStringLiteral("message")) {
            continue;
        }
        error.insert(it.key(), it.value());
    }

    QJsonObject response{
        {"ok", false},
        {"error", error},
    };

    if (!id.isUndefined()) {
        response.insert(QStringLiteral("id"), id);
    }

    return response;
}
