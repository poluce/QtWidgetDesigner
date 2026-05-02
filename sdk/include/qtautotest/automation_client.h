#pragma once

#include <qtautotest/selector.h>
#include <qtautotest/snapshot.h>

#include <QKeySequence>
#include <QUrl>
#include <QVariant>
#include <QVector>

namespace qtautotest {

enum class ErrorCode
{
    None,
    Transport,
    InvalidJson,
    MissingCommand,
    UnknownCommand,
    MissingEvents,
    UnsupportedEvent,
    InvalidSelector,
    MissingSubscriptionId,
    SubscriptionNotFound,
    NotFound,
    AmbiguousSelector,
    NotClickable,
    NotEditable,
    UnsupportedWidget,
    WindowNotFound,
    NotFocusable,
    UnknownKey,
    InvalidShortcut,
    NotScrollable,
    NotSelectable,
    NotCheckable,
    AssertionFailed,
    Timeout,
    MissingExpectation,
    InvalidRegex,
    InputTooLarge,
    Unknown,
};

QString toString(ErrorCode code);

enum class CommandKind
{
    Unknown,
    Ping,
    ListCommands,
    ListEventTypes,
    Subscribe,
    Unsubscribe,
    DescribeUi,
    DescribeSnapshot,
    DescribeObjectTree,
    DescribeLayoutTree,
    DescribeSubtree,
    DescribeStyle,
    DescribeActivePage,
    ListWindows,
    FocusWindow,
    FindWidgets,
    Click,
    SetText,
    PressKey,
    SendShortcut,
    Scroll,
    ScrollIntoView,
    SelectItem,
    ToggleCheck,
    ChooseComboOption,
    ActivateTab,
    SwitchStackedPage,
    ExpandTreeNode,
    CollapseTreeNode,
    AssertWidget,
    WaitForWidget,
    WaitForLog,
    GetLogs,
    CaptureWindow,
};

QString toString(CommandKind command);

enum class EventKind
{
    Unknown,
    TabChanged,
    ActivePageChanged,
    WindowFocusChanged,
    FocusWidgetChanged,
    ModalDialogChanged,
};

QString toString(EventKind event);

enum class ScrollDirection
{
    Up,
    Down,
    Left,
    Right,
};

enum class LogMatchMode
{
    TextContains,
    Regex,
    TextAndRegex,
};

struct TreeOptions
{
    bool visibleOnly = false;
};

struct SubtreeOptions
{
    bool layoutTree = false;
    bool visibleOnly = false;
};

struct StyleOptions
{
    bool includeChildren = false;
};

struct WaitOptions
{
    int timeoutMs = 3000;
    int pollIntervalMs = 100;
};

struct KeyPress
{
    Qt::Key key = Qt::Key_unknown;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    static KeyPress forKey(Qt::Key keyValue, Qt::KeyboardModifiers modifierValue = Qt::NoModifier)
    {
        KeyPress press;
        press.key = keyValue;
        press.modifiers = modifierValue;
        return press;
    }

    bool isValid() const { return key != Qt::Key_unknown; }
};

struct ScrollRequest
{
    Selector selector;
    ScrollDirection direction = ScrollDirection::Down;
    int amount = 120;
};

struct ChoiceTarget
{
    QString text;
    int index = -1;

    static ChoiceTarget byText(const QString& value)
    {
        ChoiceTarget target;
        target.text = value;
        return target;
    }

    static ChoiceTarget byIndex(int value)
    {
        ChoiceTarget target;
        target.index = value;
        return target;
    }

    bool hasText() const { return !text.isEmpty(); }
    bool hasIndex() const { return index >= 0; }
};

struct ListItemTarget
{
    int row = -1;
    QString text;

    static ListItemTarget byRow(int value)
    {
        ListItemTarget target;
        target.row = value;
        return target;
    }

    static ListItemTarget byText(const QString& value)
    {
        ListItemTarget target;
        target.text = value;
        return target;
    }

    bool hasRow() const { return row >= 0; }
    bool hasText() const { return !text.isEmpty(); }
};

struct TreeItemTarget
{
    QStringList path;
    QString text;

    static TreeItemTarget byPath(const QStringList& value)
    {
        TreeItemTarget target;
        target.path = value;
        return target;
    }

    static TreeItemTarget byText(const QString& value)
    {
        TreeItemTarget target;
        target.text = value;
        return target;
    }

    bool hasPath() const { return !path.isEmpty(); }
    bool hasText() const { return !text.isEmpty(); }
};

struct WidgetAssertions
{
    QString textEquals;
    QString textContains;
    QString objectName;
    QString className;
    QString windowTitleContains;
    QString riskLevel;
    bool visible = false;
    bool enabled = false;
    bool interactive = false;
    bool input = false;
    bool hasTextEquals = false;
    bool hasTextContains = false;
    bool hasObjectName = false;
    bool hasClassName = false;
    bool hasWindowTitleContains = false;
    bool hasRiskLevel = false;
    bool hasVisible = false;
    bool hasEnabled = false;
    bool hasInteractive = false;
    bool hasInput = false;

    WidgetAssertions& expectTextEquals(const QString& value) { textEquals = value; hasTextEquals = true; return *this; }
    WidgetAssertions& expectTextContains(const QString& value) { textContains = value; hasTextContains = true; return *this; }
    WidgetAssertions& expectObjectName(const QString& value) { objectName = value; hasObjectName = true; return *this; }
    WidgetAssertions& expectClassName(const QString& value) { className = value; hasClassName = true; return *this; }
    WidgetAssertions& expectWindowTitleContains(const QString& value) { windowTitleContains = value; hasWindowTitleContains = true; return *this; }
    WidgetAssertions& expectRiskLevel(const QString& value) { riskLevel = value; hasRiskLevel = true; return *this; }
    WidgetAssertions& expectVisible(bool value) { visible = value; hasVisible = true; return *this; }
    WidgetAssertions& expectEnabled(bool value) { enabled = value; hasEnabled = true; return *this; }
    WidgetAssertions& expectInteractive(bool value) { interactive = value; hasInteractive = true; return *this; }
    WidgetAssertions& expectInput(bool value) { input = value; hasInput = true; return *this; }

    bool isEmpty() const
    {
        return !hasTextEquals &&
               !hasTextContains &&
               !hasObjectName &&
               !hasClassName &&
               !hasWindowTitleContains &&
               !hasRiskLevel &&
               !hasVisible &&
               !hasEnabled &&
               !hasInteractive &&
               !hasInput;
    }
};

struct AssertionCheck
{
    QString name;
    QVariant expected;
    QVariant actual;
    bool passed = false;
};

struct WidgetCheckResult
{
    SnapshotNode widget;
    QVector<AssertionCheck> checks;
    int elapsedMs = -1;

    bool isValid() const
    {
        return widget.isValid() || !checks.isEmpty() || elapsedMs >= 0;
    }
};

struct WaitObservation
{
    SnapshotNode widget;
    QVector<AssertionCheck> checks;
    ErrorCode code = ErrorCode::None;
    QString message;
    QVector<SnapshotNode> candidates;
    bool hasWidget = false;
};

struct LogEntry
{
    QString level;
    QString message;
};

struct LogQuery
{
    QString textContains;
    QString regex;
    int limit = 200;

    static LogQuery text(const QString& value, int maxEntries = 200)
    {
        LogQuery query;
        query.textContains = value;
        query.limit = maxEntries;
        return query;
    }

    static LogQuery regexPattern(const QString& value, int maxEntries = 200)
    {
        LogQuery query;
        query.regex = value;
        query.limit = maxEntries;
        return query;
    }

    bool hasTextContains() const { return !textContains.isEmpty(); }
    bool hasRegex() const { return !regex.isEmpty(); }
    bool isValid() const { return hasTextContains() || hasRegex(); }
    LogMatchMode matchMode() const
    {
        if (hasTextContains() && hasRegex()) {
            return LogMatchMode::TextAndRegex;
        }
        return hasRegex() ? LogMatchMode::Regex : LogMatchMode::TextContains;
    }
};

struct LogMatchResult
{
    LogEntry entry;
    int elapsedMs = -1;
    int entriesScanned = 0;
};

struct ActionResult
{
    SnapshotNode widget;
    bool hasWidget = false;
    QString clickedObjectName;
    QString ref;
    QString text;
    QString selectedText;
    int index = -1;
    int row = -1;
    int column = -1;
    bool checked = false;
    bool hasChecked = false;
    bool widgetDestroyedAfterAction = false;
};

struct AutomationError
{
    ErrorCode code = ErrorCode::None;
    QString bridgeCode;
    QString message;
    QString transportError;
    QVector<SnapshotNode> candidates;
    WidgetCheckResult assertionReport;
    bool hasAssertionReport = false;
    WaitObservation lastWidgetObservation;
    bool hasLastWidgetObservation = false;
    int timeoutMs = -1;
    int pollIntervalMs = -1;
    int limit = -1;
    QString regexError;

    bool isValid() const
    {
        return code != ErrorCode::None ||
               !bridgeCode.isEmpty() ||
               !message.isEmpty() ||
               !transportError.isEmpty();
    }
};

template <typename T>
struct Result
{
    T value{};
    AutomationError error;

    bool ok() const { return !error.isValid(); }
    explicit operator bool() const { return ok(); }
};

struct OperationResult
{
    AutomationError error;

    bool ok() const { return !error.isValid(); }
    explicit operator bool() const { return ok(); }
};

class AutomationClient
{
public:
    explicit AutomationClient(QUrl bridgeUrl = QUrl(QStringLiteral("ws://127.0.0.1:49555")));

    const QUrl& bridgeUrl() const;
    void setBridgeUrl(QUrl bridgeUrl);

    OperationResult ping() const;
    Result<QVector<CommandKind>> supportedCommands() const;
    Result<QVector<EventKind>> supportedEvents() const;

    Result<SnapshotView> describeUi() const;
    Result<SnapshotView> describeSnapshot() const;
    Result<ObjectTreeView> describeObjectTree(const TreeOptions& options = TreeOptions{}) const;
    Result<LayoutTreeView> describeLayoutTree(const TreeOptions& options = TreeOptions{}) const;
    Result<SubtreeView> describeSubtree(const Selector& selector, const SubtreeOptions& options = SubtreeOptions{}) const;
    Result<StyleTreeView> describeStyle(const Selector& selector, const StyleOptions& options = StyleOptions{}) const;
    Result<ActivePageView> describeActivePage() const;
    Result<QVector<WindowInfo>> listWindows() const;
    Result<QVector<SnapshotNode>> findWidgets(const Selector& selector) const;
    Result<QVector<LogEntry>> getLogs(const LogQuery& query = LogQuery{}) const;
    Result<WindowCapture> captureWindow(const Selector& selector = Selector{}) const;

    Result<SnapshotNode> focusWindow(const Selector& selector = Selector{}) const;
    Result<ActionResult> click(const Selector& selector) const;
    Result<ActionResult> setText(const Selector& selector, const QString& text) const;
    Result<ActionResult> pressKey(const Selector& selector, const KeyPress& keyPress) const;
    Result<ActionResult> sendShortcut(const Selector& selector, const QKeySequence& shortcut) const;
    Result<ActionResult> scroll(const ScrollRequest& request) const;
    Result<ActionResult> scrollIntoView(const Selector& selector) const;
    Result<ActionResult> selectListItem(const Selector& selector, const ListItemTarget& target) const;
    Result<ActionResult> selectTreeItem(const Selector& selector, const TreeItemTarget& target) const;
    Result<ActionResult> selectTableCell(const Selector& selector, int row, int column) const;
    Result<ActionResult> setChecked(const Selector& selector, bool checked) const;
    Result<ActionResult> chooseComboOption(const Selector& selector, const ChoiceTarget& target) const;
    Result<ActionResult> activateTab(const Selector& selector, const ChoiceTarget& target) const;
    Result<ActionResult> switchStackedPage(const Selector& selector, int index) const;
    Result<ActionResult> expandTreeNode(const Selector& selector, const QStringList& path) const;
    Result<ActionResult> collapseTreeNode(const Selector& selector, const QStringList& path) const;

    Result<WidgetCheckResult> assertWidget(const Selector& selector, const WidgetAssertions& assertions = WidgetAssertions{}) const;
    Result<WidgetCheckResult> waitForWidget(const Selector& selector, const WidgetAssertions& assertions,
                                            const WaitOptions& options = WaitOptions{}) const;
    Result<LogMatchResult> waitForLog(const LogQuery& query, const WaitOptions& options = WaitOptions{}) const;

private:
    QUrl m_bridgeUrl;
};

} // namespace qtautotest
