#include <qtautotest/automation_client.h>

#include "automation_codec.h"
#include "bridge_client.h"

#include <QJsonArray>
#include <QRegularExpression>

#include <utility>

namespace qtautotest {

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

template <typename T, typename Parser>
Result<T> callAndParse(const QUrl& bridgeUrl, const QString& command, const QJsonObject& params, Parser parser)
{
    const BridgeCallResult callResult = BridgeClient(bridgeUrl).call(command, params);
    if (!callResult.transportOk) {
        return transportFailure<T>(callResult.transportError);
    }
    if (!callResult.response.value(QStringLiteral("ok")).toBool()) {
        return bridgeFailure<T>(callResult.response);
    }

    Result<T> result;
    result.value = parser(callResult.response.value(QStringLiteral("result")).toObject());
    return result;
}

Result<ActionResult> callAction(const QUrl& bridgeUrl, const QString& command, const QJsonObject& params)
{
    return callAndParse<ActionResult>(bridgeUrl, command, params, detail::parseActionResult);
}

bool logEntryMatches(const LogEntry& entry, const LogQuery& query)
{
    if (query.hasTextContains() &&
        !entry.message.contains(query.textContains, Qt::CaseInsensitive)) {
        return false;
    }

    if (query.hasRegex()) {
        const QRegularExpression expression(query.regex);
        if (!expression.isValid() || !expression.match(entry.message).hasMatch()) {
            return false;
        }
    }

    return true;
}

} // namespace

AutomationClient::AutomationClient(QUrl bridgeUrl)
    : m_bridgeUrl(std::move(bridgeUrl))
{
}

const QUrl& AutomationClient::bridgeUrl() const
{
    return m_bridgeUrl;
}

void AutomationClient::setBridgeUrl(QUrl bridgeUrl)
{
    m_bridgeUrl = std::move(bridgeUrl);
}

OperationResult AutomationClient::ping() const
{
    const BridgeCallResult callResult = BridgeClient(m_bridgeUrl).call(QStringLiteral("ping"));
    if (!callResult.transportOk) {
        return operationTransportFailure(callResult.transportError);
    }
    if (!callResult.response.value(QStringLiteral("ok")).toBool()) {
        return operationBridgeFailure(callResult.response);
    }

    return OperationResult{};
}

Result<QVector<CommandKind>> AutomationClient::supportedCommands() const
{
    return callAndParse<QVector<CommandKind>>(m_bridgeUrl, QStringLiteral("list_commands"), QJsonObject{},
                                              [](const QJsonObject& object) {
        QVector<CommandKind> commands;
        const QJsonArray array = object.value(QStringLiteral("commands")).toArray();
        commands.reserve(array.size());
        for (const QJsonValue& value : array) {
            commands.append(detail::commandKindFromString(value.toString()));
        }
        return commands;
    });
}

Result<QVector<EventKind>> AutomationClient::supportedEvents() const
{
    return callAndParse<QVector<EventKind>>(m_bridgeUrl, QStringLiteral("list_event_types"), QJsonObject{},
                                            [](const QJsonObject& object) {
        QVector<EventKind> events;
        const QJsonArray array = object.value(QStringLiteral("events")).toArray();
        events.reserve(array.size());
        for (const QJsonValue& value : array) {
            events.append(detail::eventKindFromString(value.toString()));
        }
        return events;
    });
}

Result<SnapshotView> AutomationClient::describeUi() const
{
    return callAndParse<SnapshotView>(m_bridgeUrl, QStringLiteral("describe_ui"), QJsonObject{},
                                      detail::parseSnapshotView);
}

Result<SnapshotView> AutomationClient::describeSnapshot() const
{
    return callAndParse<SnapshotView>(m_bridgeUrl, QStringLiteral("describe_snapshot"), QJsonObject{},
                                      detail::parseSnapshotView);
}

Result<ObjectTreeView> AutomationClient::describeObjectTree(const TreeOptions& options) const
{
    return callAndParse<ObjectTreeView>(m_bridgeUrl, QStringLiteral("describe_object_tree"),
                                        QJsonObject{{QStringLiteral("visibleOnly"), options.visibleOnly}},
                                        detail::parseObjectTreeView);
}

Result<LayoutTreeView> AutomationClient::describeLayoutTree(const TreeOptions& options) const
{
    return callAndParse<LayoutTreeView>(m_bridgeUrl, QStringLiteral("describe_layout_tree"),
                                        QJsonObject{{QStringLiteral("visibleOnly"), options.visibleOnly}},
                                        detail::parseLayoutTreeView);
}

Result<SubtreeView> AutomationClient::describeSubtree(const Selector& selector, const SubtreeOptions& options) const
{
    return callAndParse<SubtreeView>(m_bridgeUrl, QStringLiteral("describe_subtree"),
                                     QJsonObject{
                                         {QStringLiteral("selector"), detail::encodeSelector(selector)},
                                         {QStringLiteral("layoutTree"), options.layoutTree},
                                         {QStringLiteral("visibleOnly"), options.visibleOnly},
                                     },
                                     detail::parseSubtreeView);
}

Result<StyleTreeView> AutomationClient::describeStyle(const Selector& selector, const StyleOptions& options) const
{
    return callAndParse<StyleTreeView>(m_bridgeUrl, QStringLiteral("describe_style"),
                                       QJsonObject{
                                           {QStringLiteral("selector"), detail::encodeSelector(selector)},
                                           {QStringLiteral("includeChildren"), options.includeChildren},
                                       },
                                       detail::parseStyleTreeView);
}

Result<ActivePageView> AutomationClient::describeActivePage() const
{
    return callAndParse<ActivePageView>(m_bridgeUrl, QStringLiteral("describe_active_page"), QJsonObject{},
                                        detail::parseActivePageView);
}

Result<QVector<WindowInfo>> AutomationClient::listWindows() const
{
    return callAndParse<QVector<WindowInfo>>(m_bridgeUrl, QStringLiteral("list_windows"), QJsonObject{},
                                             [](const QJsonObject& object) {
        return detail::parseWindowInfos(object.value(QStringLiteral("windows")).toArray());
    });
}

Result<QVector<SnapshotNode>> AutomationClient::findWidgets(const Selector& selector) const
{
    return callAndParse<QVector<SnapshotNode>>(m_bridgeUrl, QStringLiteral("find_widgets"),
                                               QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}},
                                               [](const QJsonObject& object) {
        return detail::parseSnapshotNodes(object.value(QStringLiteral("matches")).toArray());
    });
}

Result<QVector<LogEntry>> AutomationClient::getLogs(const LogQuery& query) const
{
    const int limit = query.limit > 0 ? query.limit : 50;
    Result<QVector<LogEntry>> result = callAndParse<QVector<LogEntry>>(m_bridgeUrl, QStringLiteral("get_logs"),
                                                                       QJsonObject{{QStringLiteral("limit"), limit}},
                                                                       [](const QJsonObject& object) {
        return detail::parseLogEntries(object.value(QStringLiteral("entries")).toArray());
    });
    if (!result) {
        return result;
    }

    if (!query.hasTextContains() && !query.hasRegex()) {
        return result;
    }

    QVector<LogEntry> filtered;
    for (const LogEntry& entry : result.value) {
        if (logEntryMatches(entry, query)) {
            filtered.append(entry);
        }
    }
    result.value = filtered;
    return result;
}

Result<WindowCapture> AutomationClient::captureWindow(const Selector& selector) const
{
    return callAndParse<WindowCapture>(m_bridgeUrl, QStringLiteral("capture_window"),
                                       QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}},
                                       detail::parseWindowCapture);
}

Result<SnapshotNode> AutomationClient::focusWindow(const Selector& selector) const
{
    return callAndParse<SnapshotNode>(m_bridgeUrl, QStringLiteral("focus_window"),
                                      QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}},
                                      [](const QJsonObject& object) {
        return detail::parseSnapshotNode(object.value(QStringLiteral("window")).toObject());
    });
}

Result<ActionResult> AutomationClient::click(const Selector& selector) const
{
    return callAction(m_bridgeUrl, QStringLiteral("click"),
                      QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}});
}

Result<ActionResult> AutomationClient::setText(const Selector& selector, const QString& text) const
{
    return callAction(m_bridgeUrl, QStringLiteral("set_text"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("text"), text},
                      });
}

Result<ActionResult> AutomationClient::pressKey(const Selector& selector, const KeyPress& keyPress) const
{
    if (!keyPress.isValid()) {
        return bridgeFailure<ActionResult>(QJsonObject{
            {QStringLiteral("error"), QJsonObject{
                 {QStringLiteral("code"), QStringLiteral("unknown_key")},
                 {QStringLiteral("message"), QStringLiteral("KeyPress must contain a valid Qt::Key.")},
             }},
        });
    }

    return callAction(m_bridgeUrl, QStringLiteral("press_key"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("key"), detail::encodeKey(keyPress.key)},
                          {QStringLiteral("modifiers"), detail::encodeModifiers(keyPress.modifiers)},
                      });
}

Result<ActionResult> AutomationClient::sendShortcut(const Selector& selector, const QKeySequence& shortcut) const
{
    if (shortcut.count() == 0) {
        return bridgeFailure<ActionResult>(QJsonObject{
            {QStringLiteral("error"), QJsonObject{
                 {QStringLiteral("code"), QStringLiteral("invalid_shortcut")},
                 {QStringLiteral("message"), QStringLiteral("Shortcut must not be empty.")},
             }},
        });
    }

    return callAction(m_bridgeUrl, QStringLiteral("send_shortcut"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("shortcut"), shortcut.toString(QKeySequence::PortableText)},
                      });
}

Result<ActionResult> AutomationClient::scroll(const ScrollRequest& request) const
{
    return callAction(m_bridgeUrl, QStringLiteral("scroll"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(request.selector)},
                          {QStringLiteral("direction"), detail::encodeScrollDirection(request.direction)},
                          {QStringLiteral("amount"), request.amount},
                      });
}

Result<ActionResult> AutomationClient::scrollIntoView(const Selector& selector) const
{
    return callAction(m_bridgeUrl, QStringLiteral("scroll_into_view"),
                      QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}});
}

Result<ActionResult> AutomationClient::selectListItem(const Selector& selector, const ListItemTarget& target) const
{
    return callAction(m_bridgeUrl, QStringLiteral("select_item"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("options"), detail::encodeListItemTarget(target)},
                      });
}

Result<ActionResult> AutomationClient::selectTreeItem(const Selector& selector, const TreeItemTarget& target) const
{
    return callAction(m_bridgeUrl, QStringLiteral("select_item"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("options"), detail::encodeTreeItemTarget(target)},
                      });
}

Result<ActionResult> AutomationClient::selectTableCell(const Selector& selector, int row, int column) const
{
    return callAction(m_bridgeUrl, QStringLiteral("select_item"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("options"), QJsonObject{
                               {QStringLiteral("row"), row},
                               {QStringLiteral("column"), column},
                           }},
                      });
}

Result<ActionResult> AutomationClient::setChecked(const Selector& selector, bool checked) const
{
    return callAction(m_bridgeUrl, QStringLiteral("toggle_check"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("checked"), checked},
                      });
}

Result<ActionResult> AutomationClient::chooseComboOption(const Selector& selector, const ChoiceTarget& target) const
{
    QJsonObject params{
        {QStringLiteral("selector"), detail::encodeSelector(selector)},
    };
    if (target.hasText()) params.insert(QStringLiteral("text"), target.text);
    if (target.hasIndex()) params.insert(QStringLiteral("index"), target.index);
    return callAction(m_bridgeUrl, QStringLiteral("choose_combo_option"), params);
}

Result<ActionResult> AutomationClient::activateTab(const Selector& selector, const ChoiceTarget& target) const
{
    QJsonObject params{
        {QStringLiteral("selector"), detail::encodeSelector(selector)},
    };
    if (target.hasText()) params.insert(QStringLiteral("text"), target.text);
    if (target.hasIndex()) params.insert(QStringLiteral("index"), target.index);
    return callAction(m_bridgeUrl, QStringLiteral("activate_tab"), params);
}

Result<ActionResult> AutomationClient::switchStackedPage(const Selector& selector, int index) const
{
    return callAction(m_bridgeUrl, QStringLiteral("switch_stacked_page"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("index"), index},
                      });
}

Result<ActionResult> AutomationClient::expandTreeNode(const Selector& selector, const QStringList& path) const
{
    return callAction(m_bridgeUrl, QStringLiteral("expand_tree_node"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("path"), detail::encodeTreePath(path)},
                      });
}

Result<ActionResult> AutomationClient::collapseTreeNode(const Selector& selector, const QStringList& path) const
{
    return callAction(m_bridgeUrl, QStringLiteral("collapse_tree_node"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("path"), detail::encodeTreePath(path)},
                      });
}

Result<WidgetCheckResult> AutomationClient::assertWidget(const Selector& selector, const WidgetAssertions& assertions) const
{
    return callAndParse<WidgetCheckResult>(m_bridgeUrl, QStringLiteral("assert_widget"),
                                           QJsonObject{
                                               {QStringLiteral("selector"), detail::encodeSelector(selector)},
                                               {QStringLiteral("assertions"), detail::encodeWidgetAssertions(assertions)},
                                           },
                                           detail::parseWidgetCheckResult);
}

Result<WidgetCheckResult> AutomationClient::waitForWidget(const Selector& selector, const WidgetAssertions& assertions,
                                                          const WaitOptions& options) const
{
    return callAndParse<WidgetCheckResult>(m_bridgeUrl, QStringLiteral("wait_for_widget"),
                                           QJsonObject{
                                               {QStringLiteral("selector"), detail::encodeSelector(selector)},
                                               {QStringLiteral("assertions"), detail::encodeWidgetAssertions(assertions)},
                                               {QStringLiteral("timeoutMs"), options.timeoutMs},
                                               {QStringLiteral("pollIntervalMs"), options.pollIntervalMs},
                                           },
                                           detail::parseWidgetCheckResult);
}

Result<LogMatchResult> AutomationClient::waitForLog(const LogQuery& query, const WaitOptions& options) const
{
    return callAndParse<LogMatchResult>(m_bridgeUrl, QStringLiteral("wait_for_log"),
                                        QJsonObject{
                                            {QStringLiteral("textContains"), query.textContains},
                                            {QStringLiteral("regex"), query.regex},
                                            {QStringLiteral("limit"), query.limit},
                                            {QStringLiteral("timeoutMs"), options.timeoutMs},
                                            {QStringLiteral("pollIntervalMs"), options.pollIntervalMs},
                                        },
                                        detail::parseLogMatchResult);
}

} // namespace qtautotest
