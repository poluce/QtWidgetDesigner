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
Result<T> callAndParse(BridgeClient& bridgeClient, const QString& command, const QJsonObject& params, Parser parser)
{
    const BridgeCallResult callResult = bridgeClient.call(command, params);
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

Result<ActionResult> callAction(BridgeClient& bridgeClient, const QString& command, const QJsonObject& params)
{
    return callAndParse<ActionResult>(bridgeClient, command, params, detail::parseActionResult);
}

bool logEntryMatches(const LogEntry& entry, const LogQuery& query,
                     const QRegularExpression* compiledRegex = nullptr)
{
    if (query.hasTextContains() &&
        !entry.message.contains(query.textContains, Qt::CaseInsensitive)) {
        return false;
    }

    if (query.hasRegex()) {
        const QRegularExpression& expression = compiledRegex != nullptr
            ? *compiledRegex
            : QRegularExpression(query.regex);
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

AutomationClient::~AutomationClient()
{
    delete m_client;
}

AutomationClient::AutomationClient(AutomationClient&& other) noexcept
    : m_bridgeUrl(std::move(other.m_bridgeUrl))
    , m_client(other.m_client)
{
    other.m_client = nullptr;
}

AutomationClient& AutomationClient::operator=(AutomationClient&& other) noexcept
{
    if (this != &other) {
        delete m_client;
        m_bridgeUrl = std::move(other.m_bridgeUrl);
        m_client = other.m_client;
        other.m_client = nullptr;
    }
    return *this;
}

const QUrl& AutomationClient::bridgeUrl() const
{
    return m_bridgeUrl;
}

void AutomationClient::setBridgeUrl(QUrl bridgeUrl)
{
    m_bridgeUrl = std::move(bridgeUrl);
    // 下次访问 client() 时自动用新 URL 重建
    delete m_client;
    m_client = nullptr;
}

BridgeClient& AutomationClient::client()
{
    if (m_client == nullptr) {
        m_client = new BridgeClient(m_bridgeUrl);
    } else if (m_client->bridgeUrl() != m_bridgeUrl) {
        // URL 已变更，重建
        delete m_client;
        m_client = new BridgeClient(m_bridgeUrl);
    }
    return *m_client;
}

OperationResult AutomationClient::ping()
{
    const BridgeCallResult callResult = client().call(QStringLiteral("ping"));
    if (!callResult.transportOk) {
        return operationTransportFailure(callResult.transportError);
    }
    if (!callResult.response.value(QStringLiteral("ok")).toBool()) {
        return operationBridgeFailure(callResult.response);
    }

    return OperationResult{};
}

Result<QVector<CommandKind>> AutomationClient::supportedCommands()
{
    return callAndParse<QVector<CommandKind>>(client(), QStringLiteral("list_commands"), QJsonObject{},
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

Result<QVector<EventKind>> AutomationClient::supportedEvents()
{
    return callAndParse<QVector<EventKind>>(client(), QStringLiteral("list_event_types"), QJsonObject{},
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

Result<SnapshotView> AutomationClient::describeUi()
{
    return callAndParse<SnapshotView>(client(), QStringLiteral("describe_ui"), QJsonObject{},
                                       detail::parseSnapshotView);
}

Result<SnapshotView> AutomationClient::describeSnapshot()
{
    return callAndParse<SnapshotView>(client(), QStringLiteral("describe_snapshot"), QJsonObject{},
                                       detail::parseSnapshotView);
}

Result<ObjectTreeView> AutomationClient::describeObjectTree(const TreeOptions& options)
{
    return callAndParse<ObjectTreeView>(client(), QStringLiteral("describe_object_tree"),
                                         QJsonObject{{QStringLiteral("visibleOnly"), options.visibleOnly}},
                                         detail::parseObjectTreeView);
}

Result<LayoutTreeView> AutomationClient::describeLayoutTree(const TreeOptions& options)
{
    return callAndParse<LayoutTreeView>(client(), QStringLiteral("describe_layout_tree"),
                                         QJsonObject{{QStringLiteral("visibleOnly"), options.visibleOnly}},
                                         detail::parseLayoutTreeView);
}

Result<SubtreeView> AutomationClient::describeSubtree(const Selector& selector, const SubtreeOptions& options)
{
    return callAndParse<SubtreeView>(client(), QStringLiteral("describe_subtree"),
                                      QJsonObject{
                                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                                          {QStringLiteral("layoutTree"), options.layoutTree},
                                          {QStringLiteral("visibleOnly"), options.visibleOnly},
                                      },
                                      detail::parseSubtreeView);
}

Result<StyleTreeView> AutomationClient::describeStyle(const Selector& selector, const StyleOptions& options)
{
    return callAndParse<StyleTreeView>(client(), QStringLiteral("describe_style"),
                                        QJsonObject{
                                            {QStringLiteral("selector"), detail::encodeSelector(selector)},
                                            {QStringLiteral("includeChildren"), options.includeChildren},
                                        },
                                        detail::parseStyleTreeView);
}

Result<ActivePageView> AutomationClient::describeActivePage()
{
    return callAndParse<ActivePageView>(client(), QStringLiteral("describe_active_page"), QJsonObject{},
                                         detail::parseActivePageView);
}

Result<QVector<WindowInfo>> AutomationClient::listWindows()
{
    return callAndParse<QVector<WindowInfo>>(client(), QStringLiteral("list_windows"), QJsonObject{},
                                              [](const QJsonObject& object) {
        return detail::parseWindowInfos(object.value(QStringLiteral("windows")).toArray());
    });
}

Result<QVector<SnapshotNode>> AutomationClient::findWidgets(const Selector& selector)
{
    return callAndParse<QVector<SnapshotNode>>(client(), QStringLiteral("find_widgets"),
                                                QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}},
                                                [](const QJsonObject& object) {
        return detail::parseSnapshotNodes(object.value(QStringLiteral("matches")).toArray());
    });
}

Result<QVector<LogEntry>> AutomationClient::getLogs(const LogQuery& query)
{
    const int limit = query.limit > 0 ? query.limit : 50;
    Result<QVector<LogEntry>> result = callAndParse<QVector<LogEntry>>(client(), QStringLiteral("get_logs"),
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

    // 预编译正则，避免在循环中重复编译
    const QRegularExpression compiledRegex(query.hasRegex() ? query.regex : QString());
    const QRegularExpression* regexPtr = query.hasRegex() ? &compiledRegex : nullptr;

    QVector<LogEntry> filtered;
    for (const LogEntry& entry : result.value) {
        if (logEntryMatches(entry, query, regexPtr)) {
            filtered.append(entry);
        }
    }
    result.value = std::move(filtered);
    return result;
}

Result<WindowCapture> AutomationClient::captureWindow(const Selector& selector)
{
    return callAndParse<WindowCapture>(client(), QStringLiteral("capture_window"),
                                        QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}},
                                        detail::parseWindowCapture);
}

Result<SnapshotNode> AutomationClient::focusWindow(const Selector& selector)
{
    return callAndParse<SnapshotNode>(client(), QStringLiteral("focus_window"),
                                       QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}},
                                       [](const QJsonObject& object) {
        return detail::parseSnapshotNode(object.value(QStringLiteral("window")).toObject());
    });
}

Result<ActionResult> AutomationClient::click(const Selector& selector)
{
    return callAction(client(), QStringLiteral("click"),
                      QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}});
}

Result<ActionResult> AutomationClient::setText(const Selector& selector, const QString& text)
{
    return callAction(client(), QStringLiteral("set_text"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("text"), text},
                      });
}

Result<ActionResult> AutomationClient::pressKey(const Selector& selector, const KeyPress& keyPress)
{
    if (!keyPress.isValid()) {
        return bridgeFailure<ActionResult>(QJsonObject{
            {QStringLiteral("error"), QJsonObject{
                 {QStringLiteral("code"), QStringLiteral("unknown_key")},
                 {QStringLiteral("message"), QStringLiteral("KeyPress must contain a valid Qt::Key.")},
             }},
        });
    }

    return callAction(client(), QStringLiteral("press_key"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("key"), detail::encodeKey(keyPress.key)},
                          {QStringLiteral("modifiers"), detail::encodeModifiers(keyPress.modifiers)},
                      });
}

Result<ActionResult> AutomationClient::sendShortcut(const Selector& selector, const QKeySequence& shortcut)
{
    if (shortcut.count() == 0) {
        return bridgeFailure<ActionResult>(QJsonObject{
            {QStringLiteral("error"), QJsonObject{
                 {QStringLiteral("code"), QStringLiteral("invalid_shortcut")},
                 {QStringLiteral("message"), QStringLiteral("Shortcut must not be empty.")},
             }},
        });
    }

    return callAction(client(), QStringLiteral("send_shortcut"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("shortcut"), shortcut.toString(QKeySequence::PortableText)},
                      });
}

Result<ActionResult> AutomationClient::scroll(const ScrollRequest& request)
{
    return callAction(client(), QStringLiteral("scroll"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(request.selector)},
                          {QStringLiteral("direction"), detail::encodeScrollDirection(request.direction)},
                          {QStringLiteral("amount"), request.amount},
                      });
}

Result<ActionResult> AutomationClient::scrollIntoView(const Selector& selector)
{
    return callAction(client(), QStringLiteral("scroll_into_view"),
                      QJsonObject{{QStringLiteral("selector"), detail::encodeSelector(selector)}});
}

Result<ActionResult> AutomationClient::selectListItem(const Selector& selector, const ListItemTarget& target)
{
    return callAction(client(), QStringLiteral("select_item"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("options"), detail::encodeListItemTarget(target)},
                      });
}

Result<ActionResult> AutomationClient::selectTreeItem(const Selector& selector, const TreeItemTarget& target)
{
    return callAction(client(), QStringLiteral("select_item"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("options"), detail::encodeTreeItemTarget(target)},
                      });
}

Result<ActionResult> AutomationClient::selectTableCell(const Selector& selector, int row, int column)
{
    return callAction(client(), QStringLiteral("select_item"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("options"), QJsonObject{
                               {QStringLiteral("row"), row},
                               {QStringLiteral("column"), column},
                           }},
                      });
}

Result<ActionResult> AutomationClient::setChecked(const Selector& selector, bool checked)
{
    return callAction(client(), QStringLiteral("toggle_check"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("checked"), checked},
                      });
}

Result<ActionResult> AutomationClient::chooseComboOption(const Selector& selector, const ChoiceTarget& target)
{
    QJsonObject params{
        {QStringLiteral("selector"), detail::encodeSelector(selector)},
    };
    if (target.hasText()) params.insert(QStringLiteral("text"), target.text);
    if (target.hasIndex()) params.insert(QStringLiteral("index"), target.index);
    return callAction(client(), QStringLiteral("choose_combo_option"), params);
}

Result<ActionResult> AutomationClient::activateTab(const Selector& selector, const ChoiceTarget& target)
{
    QJsonObject params{
        {QStringLiteral("selector"), detail::encodeSelector(selector)},
    };
    if (target.hasText()) params.insert(QStringLiteral("text"), target.text);
    if (target.hasIndex()) params.insert(QStringLiteral("index"), target.index);
    return callAction(client(), QStringLiteral("activate_tab"), params);
}

Result<ActionResult> AutomationClient::switchStackedPage(const Selector& selector, int index)
{
    return callAction(client(), QStringLiteral("switch_stacked_page"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("index"), index},
                      });
}

Result<ActionResult> AutomationClient::expandTreeNode(const Selector& selector, const QStringList& path)
{
    return callAction(client(), QStringLiteral("expand_tree_node"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("path"), detail::encodeTreePath(path)},
                      });
}

Result<ActionResult> AutomationClient::collapseTreeNode(const Selector& selector, const QStringList& path)
{
    return callAction(client(), QStringLiteral("collapse_tree_node"),
                      QJsonObject{
                          {QStringLiteral("selector"), detail::encodeSelector(selector)},
                          {QStringLiteral("path"), detail::encodeTreePath(path)},
                      });
}

Result<WidgetCheckResult> AutomationClient::assertWidget(const Selector& selector, const WidgetAssertions& assertions)
{
    return callAndParse<WidgetCheckResult>(client(), QStringLiteral("assert_widget"),
                                            QJsonObject{
                                                {QStringLiteral("selector"), detail::encodeSelector(selector)},
                                                {QStringLiteral("assertions"), detail::encodeWidgetAssertions(assertions)},
                                            },
                                            detail::parseWidgetCheckResult);
}

Result<WidgetCheckResult> AutomationClient::waitForWidget(const Selector& selector, const WidgetAssertions& assertions,
                                                          const WaitOptions& options)
{
    return callAndParse<WidgetCheckResult>(client(), QStringLiteral("wait_for_widget"),
                                            QJsonObject{
                                                {QStringLiteral("selector"), detail::encodeSelector(selector)},
                                                {QStringLiteral("assertions"), detail::encodeWidgetAssertions(assertions)},
                                                {QStringLiteral("timeoutMs"), options.timeoutMs},
                                                {QStringLiteral("pollIntervalMs"), options.pollIntervalMs},
                                            },
                                            detail::parseWidgetCheckResult);
}

Result<LogMatchResult> AutomationClient::waitForLog(const LogQuery& query, const WaitOptions& options)
{
    return callAndParse<LogMatchResult>(client(), QStringLiteral("wait_for_log"),
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
