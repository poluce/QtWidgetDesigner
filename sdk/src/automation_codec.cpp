#include "automation_codec.h"

#include <QImage>
#include <QJsonDocument>
#include <QVariant>

namespace qtautotest {

QString toString(ErrorCode code)
{
    switch (code) {
    case ErrorCode::None: return QStringLiteral("none");
    case ErrorCode::Transport: return QStringLiteral("transport");
    case ErrorCode::InvalidJson: return QStringLiteral("invalid_json");
    case ErrorCode::MissingCommand: return QStringLiteral("missing_command");
    case ErrorCode::UnknownCommand: return QStringLiteral("unknown_command");
    case ErrorCode::MissingEvents: return QStringLiteral("missing_events");
    case ErrorCode::UnsupportedEvent: return QStringLiteral("unsupported_event");
    case ErrorCode::InvalidSelector: return QStringLiteral("invalid_selector");
    case ErrorCode::MissingSubscriptionId: return QStringLiteral("missing_subscription_id");
    case ErrorCode::SubscriptionNotFound: return QStringLiteral("subscription_not_found");
    case ErrorCode::NotFound: return QStringLiteral("not_found");
    case ErrorCode::AmbiguousSelector: return QStringLiteral("ambiguous_selector");
    case ErrorCode::NotClickable: return QStringLiteral("not_clickable");
    case ErrorCode::NotEditable: return QStringLiteral("not_editable");
    case ErrorCode::UnsupportedWidget: return QStringLiteral("unsupported_widget");
    case ErrorCode::WindowNotFound: return QStringLiteral("window_not_found");
    case ErrorCode::NotFocusable: return QStringLiteral("not_focusable");
    case ErrorCode::UnknownKey: return QStringLiteral("unknown_key");
    case ErrorCode::InvalidShortcut: return QStringLiteral("invalid_shortcut");
    case ErrorCode::NotScrollable: return QStringLiteral("not_scrollable");
    case ErrorCode::NotSelectable: return QStringLiteral("not_selectable");
    case ErrorCode::NotCheckable: return QStringLiteral("not_checkable");
    case ErrorCode::AssertionFailed: return QStringLiteral("assertion_failed");
    case ErrorCode::Timeout: return QStringLiteral("timeout");
    case ErrorCode::MissingExpectation: return QStringLiteral("missing_expectation");
    case ErrorCode::InvalidRegex: return QStringLiteral("invalid_regex");
    case ErrorCode::InputTooLarge: return QStringLiteral("input_too_large");
    case ErrorCode::Unknown: return QStringLiteral("unknown");
    }

    return QStringLiteral("unknown");
}

QString toString(CommandKind command)
{
    switch (command) {
    case CommandKind::Ping: return QStringLiteral("ping");
    case CommandKind::ListCommands: return QStringLiteral("list_commands");
    case CommandKind::ListEventTypes: return QStringLiteral("list_event_types");
    case CommandKind::Subscribe: return QStringLiteral("subscribe");
    case CommandKind::Unsubscribe: return QStringLiteral("unsubscribe");
    case CommandKind::DescribeUi: return QStringLiteral("describe_ui");
    case CommandKind::DescribeSnapshot: return QStringLiteral("describe_snapshot");
    case CommandKind::DescribeObjectTree: return QStringLiteral("describe_object_tree");
    case CommandKind::DescribeLayoutTree: return QStringLiteral("describe_layout_tree");
    case CommandKind::DescribeSubtree: return QStringLiteral("describe_subtree");
    case CommandKind::DescribeStyle: return QStringLiteral("describe_style");
    case CommandKind::DescribeActivePage: return QStringLiteral("describe_active_page");
    case CommandKind::ListWindows: return QStringLiteral("list_windows");
    case CommandKind::FocusWindow: return QStringLiteral("focus_window");
    case CommandKind::FindWidgets: return QStringLiteral("find_widgets");
    case CommandKind::Click: return QStringLiteral("click");
    case CommandKind::SetText: return QStringLiteral("set_text");
    case CommandKind::PressKey: return QStringLiteral("press_key");
    case CommandKind::SendShortcut: return QStringLiteral("send_shortcut");
    case CommandKind::Scroll: return QStringLiteral("scroll");
    case CommandKind::ScrollIntoView: return QStringLiteral("scroll_into_view");
    case CommandKind::SelectItem: return QStringLiteral("select_item");
    case CommandKind::ToggleCheck: return QStringLiteral("toggle_check");
    case CommandKind::ChooseComboOption: return QStringLiteral("choose_combo_option");
    case CommandKind::ActivateTab: return QStringLiteral("activate_tab");
    case CommandKind::SwitchStackedPage: return QStringLiteral("switch_stacked_page");
    case CommandKind::ExpandTreeNode: return QStringLiteral("expand_tree_node");
    case CommandKind::CollapseTreeNode: return QStringLiteral("collapse_tree_node");
    case CommandKind::AssertWidget: return QStringLiteral("assert_widget");
    case CommandKind::WaitForWidget: return QStringLiteral("wait_for_widget");
    case CommandKind::WaitForLog: return QStringLiteral("wait_for_log");
    case CommandKind::GetLogs: return QStringLiteral("get_logs");
    case CommandKind::CaptureWindow: return QStringLiteral("capture_window");
    case CommandKind::Unknown: return QStringLiteral("unknown");
    }

    return QStringLiteral("unknown");
}

QString toString(EventKind event)
{
    switch (event) {
    case EventKind::TabChanged: return QStringLiteral("tab_changed");
    case EventKind::ActivePageChanged: return QStringLiteral("active_page_changed");
    case EventKind::WindowFocusChanged: return QStringLiteral("window_focus_changed");
    case EventKind::FocusWidgetChanged: return QStringLiteral("focus_widget_changed");
    case EventKind::ModalDialogChanged: return QStringLiteral("modal_dialog_changed");
    case EventKind::Unknown: return QStringLiteral("unknown");
    }

    return QStringLiteral("unknown");
}

} // namespace qtautotest

namespace {

QVariant parseVariant(const QJsonValue& value)
{
    if (value.isObject()) {
        return value.toObject().toVariantMap();
    }
    if (value.isArray()) {
        return value.toArray().toVariantList();
    }
    return value.toVariant();
}

qtautotest::IntRect parseRect(const QJsonObject& object)
{
    qtautotest::IntRect rect;
    rect.x = object.value(QStringLiteral("x")).toInt();
    rect.y = object.value(QStringLiteral("y")).toInt();
    rect.width = object.value(QStringLiteral("width")).toInt();
    rect.height = object.value(QStringLiteral("height")).toInt();
    return rect;
}

qtautotest::BoxMargins parseMargins(const QJsonObject& object)
{
    qtautotest::BoxMargins margins;
    margins.left = object.value(QStringLiteral("left")).toInt();
    margins.top = object.value(QStringLiteral("top")).toInt();
    margins.right = object.value(QStringLiteral("right")).toInt();
    margins.bottom = object.value(QStringLiteral("bottom")).toInt();
    return margins;
}

qtautotest::ColorValue parseColorValueImpl(const QJsonObject& object)
{
    qtautotest::ColorValue color;
    color.valid = object.value(QStringLiteral("valid")).toBool();
    color.hex = object.value(QStringLiteral("hex")).toString();
    const QJsonObject rgba = object.value(QStringLiteral("rgba")).toObject();
    color.red = rgba.value(QStringLiteral("r")).toInt();
    color.green = rgba.value(QStringLiteral("g")).toInt();
    color.blue = rgba.value(QStringLiteral("b")).toInt();
    color.alpha = rgba.value(QStringLiteral("a")).toInt(255);
    return color;
}

QMap<QString, qtautotest::ColorValue> parseColorMap(const QJsonObject& object)
{
    QMap<QString, qtautotest::ColorValue> colors;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (it.value().isObject()) {
            colors.insert(it.key(), parseColorValueImpl(it.value().toObject()));
        }
    }
    return colors;
}

qtautotest::PaletteGroup parsePaletteGroup(const QJsonObject& object)
{
    qtautotest::PaletteGroup group;
    group.colors = parseColorMap(object);
    return group;
}

} // namespace

namespace qtautotest::detail {

ErrorCode errorCodeFromString(const QString& code)
{
    if (code == QStringLiteral("invalid_json")) return ErrorCode::InvalidJson;
    if (code == QStringLiteral("missing_command")) return ErrorCode::MissingCommand;
    if (code == QStringLiteral("unknown_command")) return ErrorCode::UnknownCommand;
    if (code == QStringLiteral("missing_events")) return ErrorCode::MissingEvents;
    if (code == QStringLiteral("unsupported_event")) return ErrorCode::UnsupportedEvent;
    if (code == QStringLiteral("invalid_selector")) return ErrorCode::InvalidSelector;
    if (code == QStringLiteral("missing_subscription_id")) return ErrorCode::MissingSubscriptionId;
    if (code == QStringLiteral("subscription_not_found")) return ErrorCode::SubscriptionNotFound;
    if (code == QStringLiteral("not_found")) return ErrorCode::NotFound;
    if (code == QStringLiteral("ambiguous_selector")) return ErrorCode::AmbiguousSelector;
    if (code == QStringLiteral("not_clickable")) return ErrorCode::NotClickable;
    if (code == QStringLiteral("not_editable")) return ErrorCode::NotEditable;
    if (code == QStringLiteral("unsupported_widget")) return ErrorCode::UnsupportedWidget;
    if (code == QStringLiteral("window_not_found")) return ErrorCode::WindowNotFound;
    if (code == QStringLiteral("not_focusable")) return ErrorCode::NotFocusable;
    if (code == QStringLiteral("unknown_key")) return ErrorCode::UnknownKey;
    if (code == QStringLiteral("invalid_shortcut")) return ErrorCode::InvalidShortcut;
    if (code == QStringLiteral("not_scrollable")) return ErrorCode::NotScrollable;
    if (code == QStringLiteral("not_selectable")) return ErrorCode::NotSelectable;
    if (code == QStringLiteral("not_checkable")) return ErrorCode::NotCheckable;
    if (code == QStringLiteral("assertion_failed")) return ErrorCode::AssertionFailed;
    if (code == QStringLiteral("timeout")) return ErrorCode::Timeout;
    if (code == QStringLiteral("missing_expectation")) return ErrorCode::MissingExpectation;
    if (code == QStringLiteral("invalid_regex")) return ErrorCode::InvalidRegex;
    if (code == QStringLiteral("input_too_large")) return ErrorCode::InputTooLarge;
    return code.isEmpty() ? ErrorCode::None : ErrorCode::Unknown;
}

CommandKind commandKindFromString(const QString& command)
{
    if (command == QStringLiteral("ping")) return CommandKind::Ping;
    if (command == QStringLiteral("list_commands")) return CommandKind::ListCommands;
    if (command == QStringLiteral("list_event_types")) return CommandKind::ListEventTypes;
    if (command == QStringLiteral("subscribe")) return CommandKind::Subscribe;
    if (command == QStringLiteral("unsubscribe")) return CommandKind::Unsubscribe;
    if (command == QStringLiteral("describe_ui")) return CommandKind::DescribeUi;
    if (command == QStringLiteral("describe_snapshot")) return CommandKind::DescribeSnapshot;
    if (command == QStringLiteral("describe_object_tree")) return CommandKind::DescribeObjectTree;
    if (command == QStringLiteral("describe_layout_tree")) return CommandKind::DescribeLayoutTree;
    if (command == QStringLiteral("describe_subtree")) return CommandKind::DescribeSubtree;
    if (command == QStringLiteral("describe_style")) return CommandKind::DescribeStyle;
    if (command == QStringLiteral("describe_active_page")) return CommandKind::DescribeActivePage;
    if (command == QStringLiteral("list_windows")) return CommandKind::ListWindows;
    if (command == QStringLiteral("focus_window")) return CommandKind::FocusWindow;
    if (command == QStringLiteral("find_widgets")) return CommandKind::FindWidgets;
    if (command == QStringLiteral("click")) return CommandKind::Click;
    if (command == QStringLiteral("set_text")) return CommandKind::SetText;
    if (command == QStringLiteral("press_key")) return CommandKind::PressKey;
    if (command == QStringLiteral("send_shortcut")) return CommandKind::SendShortcut;
    if (command == QStringLiteral("scroll")) return CommandKind::Scroll;
    if (command == QStringLiteral("scroll_into_view")) return CommandKind::ScrollIntoView;
    if (command == QStringLiteral("select_item")) return CommandKind::SelectItem;
    if (command == QStringLiteral("toggle_check")) return CommandKind::ToggleCheck;
    if (command == QStringLiteral("choose_combo_option")) return CommandKind::ChooseComboOption;
    if (command == QStringLiteral("activate_tab")) return CommandKind::ActivateTab;
    if (command == QStringLiteral("switch_stacked_page")) return CommandKind::SwitchStackedPage;
    if (command == QStringLiteral("expand_tree_node")) return CommandKind::ExpandTreeNode;
    if (command == QStringLiteral("collapse_tree_node")) return CommandKind::CollapseTreeNode;
    if (command == QStringLiteral("assert_widget")) return CommandKind::AssertWidget;
    if (command == QStringLiteral("wait_for_widget")) return CommandKind::WaitForWidget;
    if (command == QStringLiteral("wait_for_log")) return CommandKind::WaitForLog;
    if (command == QStringLiteral("get_logs")) return CommandKind::GetLogs;
    if (command == QStringLiteral("capture_window")) return CommandKind::CaptureWindow;
    return CommandKind::Unknown;
}

EventKind eventKindFromString(const QString& event)
{
    if (event == QStringLiteral("tab_changed")) return EventKind::TabChanged;
    if (event == QStringLiteral("active_page_changed")) return EventKind::ActivePageChanged;
    if (event == QStringLiteral("window_focus_changed")) return EventKind::WindowFocusChanged;
    if (event == QStringLiteral("focus_widget_changed")) return EventKind::FocusWidgetChanged;
    if (event == QStringLiteral("modal_dialog_changed")) return EventKind::ModalDialogChanged;
    return EventKind::Unknown;
}

ModalDialogState modalDialogStateFromString(const QString& state)
{
    if (state == QStringLiteral("opened")) return ModalDialogState::Opened;
    if (state == QStringLiteral("closed")) return ModalDialogState::Closed;
    return ModalDialogState::Unknown;
}

QJsonObject encodeSelector(const Selector& selector)
{
    QJsonObject object;
    if (selector.hasRef()) object.insert(QStringLiteral("ref"), selector.ref());
    if (selector.hasPath()) object.insert(QStringLiteral("path"), selector.path());
    if (selector.hasObjectName()) object.insert(QStringLiteral("objectName"), selector.objectName());
    if (selector.hasClassName()) object.insert(QStringLiteral("className"), selector.className());
    if (selector.hasTextEquals()) object.insert(QStringLiteral("textEquals"), selector.textEquals());
    if (selector.hasTextContains()) object.insert(QStringLiteral("textContains"), selector.textContains());
    if (selector.hasPlaceholderText()) object.insert(QStringLiteral("placeholderText"), selector.placeholderText());
    if (selector.hasWindowObjectName()) object.insert(QStringLiteral("windowObjectName"), selector.windowObjectName());
    if (selector.hasWindowTitleContains()) object.insert(QStringLiteral("windowTitleContains"), selector.windowTitleContains());
    if (selector.hasAncestorRef()) object.insert(QStringLiteral("ancestorRef"), selector.ancestorRef());
    if (selector.hasAncestorObjectName()) object.insert(QStringLiteral("ancestorObjectName"), selector.ancestorObjectName());
    if (selector.hasCurrentPageOnly()) object.insert(QStringLiteral("currentPageOnly"), selector.currentPageOnly());
    if (selector.hasActiveWindow()) object.insert(QStringLiteral("isActiveWindow"), selector.activeWindow());
    if (selector.hasVisible()) object.insert(QStringLiteral("visible"), selector.visible());
    if (selector.hasEnabled()) object.insert(QStringLiteral("enabled"), selector.enabled());
    if (selector.hasIndexInParent()) object.insert(QStringLiteral("indexInParent"), selector.indexInParent());
    return object;
}

QJsonArray encodeSelectors(const QVector<Selector>& selectors)
{
    QJsonArray array;
    for (const Selector& selector : selectors) {
        array.append(encodeSelector(selector));
    }
    return array;
}

QJsonObject encodeWidgetAssertions(const WidgetAssertions& assertions)
{
    QJsonObject object;
    if (assertions.hasTextEquals) object.insert(QStringLiteral("textEquals"), assertions.textEquals);
    if (assertions.hasTextContains) object.insert(QStringLiteral("textContains"), assertions.textContains);
    if (assertions.hasObjectName) object.insert(QStringLiteral("objectName"), assertions.objectName);
    if (assertions.hasClassName) object.insert(QStringLiteral("className"), assertions.className);
    if (assertions.hasWindowTitleContains) object.insert(QStringLiteral("windowTitleContains"), assertions.windowTitleContains);
    if (assertions.hasRiskLevel) object.insert(QStringLiteral("riskLevel"), assertions.riskLevel);
    if (assertions.hasVisible) object.insert(QStringLiteral("visible"), assertions.visible);
    if (assertions.hasEnabled) object.insert(QStringLiteral("enabled"), assertions.enabled);
    if (assertions.hasInteractive) object.insert(QStringLiteral("isInteractive"), assertions.interactive);
    if (assertions.hasInput) object.insert(QStringLiteral("isInput"), assertions.input);
    return object;
}

QJsonObject encodeListItemTarget(const ListItemTarget& target)
{
    QJsonObject object;
    if (target.hasRow()) object.insert(QStringLiteral("row"), target.row);
    if (target.hasText()) object.insert(QStringLiteral("text"), target.text);
    return object;
}

QJsonObject encodeTreeItemTarget(const TreeItemTarget& target)
{
    QJsonObject object;
    if (target.hasPath()) object.insert(QStringLiteral("path"), encodeTreePath(target.path));
    if (target.hasText()) object.insert(QStringLiteral("text"), target.text);
    return object;
}

QJsonArray encodeTreePath(const QStringList& path)
{
    QJsonArray array;
    for (const QString& segment : path) {
        array.append(segment);
    }
    return array;
}

QString encodeScrollDirection(ScrollDirection direction)
{
    switch (direction) {
    case ScrollDirection::Up: return QStringLiteral("up");
    case ScrollDirection::Down: return QStringLiteral("down");
    case ScrollDirection::Left: return QStringLiteral("left");
    case ScrollDirection::Right: return QStringLiteral("right");
    }

    Q_UNREACHABLE();
    return QStringLiteral("down");
}

QString encodeKey(Qt::Key key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return QString(QChar(static_cast<char>('A' + (key - Qt::Key_A))));
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString(QChar(static_cast<char>('0' + (key - Qt::Key_0))));
    }

    switch (key) {
    case Qt::Key_Tab: return QStringLiteral("Tab");
    case Qt::Key_Return: return QStringLiteral("Return");
    case Qt::Key_Enter: return QStringLiteral("Enter");
    case Qt::Key_Escape: return QStringLiteral("Escape");
    case Qt::Key_Space: return QStringLiteral("Space");
    case Qt::Key_Backspace: return QStringLiteral("Backspace");
    case Qt::Key_Delete: return QStringLiteral("Delete");
    case Qt::Key_Insert: return QStringLiteral("Insert");
    case Qt::Key_Up: return QStringLiteral("Up");
    case Qt::Key_Down: return QStringLiteral("Down");
    case Qt::Key_Left: return QStringLiteral("Left");
    case Qt::Key_Right: return QStringLiteral("Right");
    case Qt::Key_Home: return QStringLiteral("Home");
    case Qt::Key_End: return QStringLiteral("End");
    case Qt::Key_PageUp: return QStringLiteral("PageUp");
    case Qt::Key_PageDown: return QStringLiteral("PageDown");
    case Qt::Key_F1: return QStringLiteral("F1");
    case Qt::Key_F2: return QStringLiteral("F2");
    case Qt::Key_F3: return QStringLiteral("F3");
    case Qt::Key_F4: return QStringLiteral("F4");
    case Qt::Key_F5: return QStringLiteral("F5");
    case Qt::Key_F6: return QStringLiteral("F6");
    case Qt::Key_F7: return QStringLiteral("F7");
    case Qt::Key_F8: return QStringLiteral("F8");
    case Qt::Key_F9: return QStringLiteral("F9");
    case Qt::Key_F10: return QStringLiteral("F10");
    case Qt::Key_F11: return QStringLiteral("F11");
    case Qt::Key_F12: return QStringLiteral("F12");
    default: return QString();
    }
}

QString encodeModifiers(Qt::KeyboardModifiers modifiers)
{
    QStringList parts;
    if (modifiers.testFlag(Qt::ControlModifier)) parts.append(QStringLiteral("Ctrl"));
    if (modifiers.testFlag(Qt::ShiftModifier)) parts.append(QStringLiteral("Shift"));
    if (modifiers.testFlag(Qt::AltModifier)) parts.append(QStringLiteral("Alt"));
    if (modifiers.testFlag(Qt::MetaModifier)) parts.append(QStringLiteral("Meta"));
    return parts.join(QStringLiteral("+"));
}

WidgetInfo parseWidgetInfo(const QJsonObject& object)
{
    WidgetInfo info;
    info.ref = object.value(QStringLiteral("ref")).toString();
    info.objectName = object.value(QStringLiteral("objectName")).toString();
    info.className = object.value(QStringLiteral("className")).toString();
    info.text = object.value(QStringLiteral("text")).toString();
    info.placeholderText = object.value(QStringLiteral("placeholderText")).toString();
    info.windowTitle = object.value(QStringLiteral("windowTitle")).toString();
    info.path = object.value(QStringLiteral("path")).toString();
    info.pageContainerType = object.value(QStringLiteral("pageContainerType")).toString();
    info.riskLevel = object.value(QStringLiteral("riskLevel")).toString();
    info.visible = object.value(QStringLiteral("visible")).toBool();
    info.enabled = object.value(QStringLiteral("enabled")).toBool();
    info.interactive = object.value(QStringLiteral("isInteractive")).toBool();
    info.input = object.value(QStringLiteral("isInput")).toBool();
    info.container = object.value(QStringLiteral("isContainer")).toBool();
    info.currentPage = object.value(QStringLiteral("isCurrentPage")).toBool();
    info.hasFocus = object.value(QStringLiteral("hasFocus")).toBool();
    info.activeWindow = object.value(QStringLiteral("isActiveWindow")).toBool();
    info.depth = object.value(QStringLiteral("depth")).toInt();
    info.indexInParent = object.value(QStringLiteral("indexInParent")).toInt(-1);
    info.geometry = parseRect(object.value(QStringLiteral("geometry")).toObject());
    const QJsonArray actions = object.value(QStringLiteral("availableActions")).toArray();
    for (const QJsonValue& value : actions) {
        info.availableActions.append(value.toString());
    }
    return info;
}

SnapshotNode parseSnapshotNode(const QJsonObject& object)
{
    SnapshotNode node;
    static_cast<WidgetInfo&>(node) = parseWidgetInfo(object);
    const QJsonArray children = object.value(QStringLiteral("children")).toArray();
    node.children.reserve(children.size());
    for (const QJsonValue& value : children) {
        node.children.push_back(std::make_shared<SnapshotNode>(parseSnapshotNode(value.toObject())));
    }
    return node;
}

QVector<SnapshotNode> parseSnapshotNodes(const QJsonArray& array)
{
    QVector<SnapshotNode> nodes;
    nodes.reserve(array.size());
    for (const QJsonValue& value : array) {
        nodes.append(parseSnapshotNode(value.toObject()));
    }
    return nodes;
}

WindowInfo parseWindowInfo(const QJsonObject& object)
{
    WindowInfo window;
    static_cast<WidgetInfo&>(window) = parseWidgetInfo(object);
    window.active = object.value(QStringLiteral("active")).toBool();
    window.modal = object.value(QStringLiteral("modal")).toBool();
    return window;
}

QVector<WindowInfo> parseWindowInfos(const QJsonArray& array)
{
    QVector<WindowInfo> windows;
    windows.reserve(array.size());
    for (const QJsonValue& value : array) {
        windows.append(parseWindowInfo(value.toObject()));
    }
    return windows;
}

LayoutNode parseLayoutNode(const QJsonObject& object)
{
    LayoutNode node;
    node.ref = object.value(QStringLiteral("ref")).toString();
    node.ownerRef = object.value(QStringLiteral("ownerRef")).toString();
    node.objectName = object.value(QStringLiteral("objectName")).toString();
    node.className = object.value(QStringLiteral("className")).toString();
    node.layoutType = object.value(QStringLiteral("layoutType")).toString();
    node.layoutRole = object.value(QStringLiteral("layoutRole")).toString();
    node.alignment = object.value(QStringLiteral("alignment")).toString();
    node.pageContainerType = object.value(QStringLiteral("pageContainerType")).toString();
    node.indexInParent = object.value(QStringLiteral("indexInParent")).toInt(-1);
    node.row = object.value(QStringLiteral("row")).toInt(-1);
    node.column = object.value(QStringLiteral("column")).toInt(-1);
    node.rowSpan = object.value(QStringLiteral("rowSpan")).toInt(1);
    node.columnSpan = object.value(QStringLiteral("columnSpan")).toInt(1);
    node.stretch = object.value(QStringLiteral("stretch")).toInt(-1);
    node.margins = parseMargins(object.value(QStringLiteral("margins")).toObject());
    node.spacing = object.value(QStringLiteral("spacing")).toInt(-1);
    node.scrollArea = object.value(QStringLiteral("isScrollArea")).toBool();
    node.currentPage = object.value(QStringLiteral("isCurrentPage")).toBool();
    const QJsonArray children = object.value(QStringLiteral("children")).toArray();
    node.children.reserve(children.size());
    for (const QJsonValue& value : children) {
        node.children.push_back(std::make_shared<LayoutNode>(parseLayoutNode(value.toObject())));
    }
    return node;
}

SnapshotView parseSnapshotView(const QJsonObject& object)
{
    SnapshotView view;
    view.windows = parseSnapshotNodes(object.value(QStringLiteral("windows")).toArray());
    view.topLevelWindows = parseWindowInfos(object.value(QStringLiteral("topLevelWindows")).toArray());
    view.widgetCount = object.value(QStringLiteral("widgetCount")).toInt();
    if (object.contains(QStringLiteral("focusWidget"))) {
        view.focusWidget = parseSnapshotNode(object.value(QStringLiteral("focusWidget")).toObject());
        view.hasFocusWidget = true;
    }
    if (object.contains(QStringLiteral("activeWindow"))) {
        view.activeWindow = parseSnapshotNode(object.value(QStringLiteral("activeWindow")).toObject());
        view.hasActiveWindow = true;
    }
    if (object.contains(QStringLiteral("visibleSubtree"))) {
        view.visibleSubtree = parseSnapshotNode(object.value(QStringLiteral("visibleSubtree")).toObject());
        view.hasVisibleSubtree = true;
    }
    if (object.contains(QStringLiteral("modalDialog"))) {
        view.modalDialog = parseSnapshotNode(object.value(QStringLiteral("modalDialog")).toObject());
        view.hasModalDialog = true;
    }
    if (object.contains(QStringLiteral("activePage"))) {
        view.activePage = parseSnapshotNode(object.value(QStringLiteral("activePage")).toObject());
        view.hasActivePage = true;
    }
    return view;
}

ObjectTreeView parseObjectTreeView(const QJsonObject& object)
{
    ObjectTreeView view;
    view.visibleOnly = object.value(QStringLiteral("visibleOnly")).toBool();
    view.windows = parseSnapshotNodes(object.value(QStringLiteral("windows")).toArray());
    return view;
}

LayoutTreeView parseLayoutTreeView(const QJsonObject& object)
{
    LayoutTreeView view;
    view.visibleOnly = object.value(QStringLiteral("visibleOnly")).toBool();
    const QJsonArray windows = object.value(QStringLiteral("windows")).toArray();
    view.windows.reserve(windows.size());
    for (const QJsonValue& value : windows) {
        view.windows.append(parseLayoutNode(value.toObject()));
    }
    return view;
}

SubtreeView parseSubtreeView(const QJsonObject& object)
{
    SubtreeView view;
    const QString treeType = object.value(QStringLiteral("treeType")).toString();
    view.visibleOnly = object.value(QStringLiteral("visibleOnly")).toBool();
    if (treeType == QStringLiteral("object")) {
        view.treeKind = TreeKind::Object;
        view.objectRoot = parseSnapshotNode(object.value(QStringLiteral("root")).toObject());
        view.hasObjectRoot = true;
    } else if (treeType == QStringLiteral("layout")) {
        view.treeKind = TreeKind::Layout;
        view.layoutRoot = parseLayoutNode(object.value(QStringLiteral("root")).toObject());
        view.hasLayoutRoot = true;
    } else {
        view.treeKind = TreeKind::Unknown;
    }
    return view;
}

namespace {

StyleRoles parseStyleRoles(const QJsonObject& object)
{
    StyleRoles roles;
    roles.foregroundRole = object.value(QStringLiteral("foregroundRole")).toString();
    roles.foregroundColor = parseColorValueImpl(object.value(QStringLiteral("foregroundColor")).toObject());
    roles.backgroundRole = object.value(QStringLiteral("backgroundRole")).toString();
    roles.backgroundColor = parseColorValueImpl(object.value(QStringLiteral("backgroundColor")).toObject());
    return roles;
}

StyleAttachmentView parseStyleAttachmentViewValue(const QJsonObject& object);

StyleView parseStyleViewValue(const QJsonObject& object)
{
    StyleView view;
    view.styleSheet = object.value(QStringLiteral("styleSheet")).toString();
    view.inheritsStyleSheet = object.value(QStringLiteral("inheritsStyleSheet")).toBool();
    view.autoFillBackground = object.value(QStringLiteral("autoFillBackground")).toBool();
    view.currentColorGroup = object.value(QStringLiteral("currentColorGroup")).toString();
    view.roles = parseStyleRoles(object.value(QStringLiteral("roles")).toObject());
    const QJsonObject palette = object.value(QStringLiteral("palette")).toObject();
    view.currentPalette = parsePaletteGroup(palette.value(QStringLiteral("current")).toObject());
    view.activePalette = parsePaletteGroup(palette.value(QStringLiteral("active")).toObject());
    view.inactivePalette = parsePaletteGroup(palette.value(QStringLiteral("inactive")).toObject());
    view.disabledPalette = parsePaletteGroup(palette.value(QStringLiteral("disabled")).toObject());
    view.classHints = parseColorMap(object.value(QStringLiteral("classHints")).toObject());

    const auto attach = [&](const char* key, StyleAttachmentView& target, bool& hasTarget) {
        const QJsonObject attachmentObject = object.value(QLatin1String(key)).toObject();
        if (attachmentObject.isEmpty()) {
            hasTarget = false;
            return;
        }
        target = parseStyleAttachmentViewValue(attachmentObject);
        hasTarget = target.isValid();
    };

    attach("viewport", view.viewport, view.hasViewport);
    attach("header", view.header, view.hasHeader);
    attach("horizontalHeader", view.horizontalHeader, view.hasHorizontalHeader);
    attach("verticalHeader", view.verticalHeader, view.hasVerticalHeader);
    return view;
}

StyleAttachmentView parseStyleAttachmentViewValue(const QJsonObject& object)
{
    StyleAttachmentView attachment;
    attachment.widget = parseWidgetInfo(object.value(QStringLiteral("widget")).toObject());
    const StyleView style = parseStyleViewValue(object.value(QStringLiteral("style")).toObject());
    attachment.styleSheet = style.styleSheet;
    attachment.inheritsStyleSheet = style.inheritsStyleSheet;
    attachment.autoFillBackground = style.autoFillBackground;
    attachment.currentColorGroup = style.currentColorGroup;
    attachment.roles = style.roles;
    attachment.currentPalette = style.currentPalette;
    attachment.activePalette = style.activePalette;
    attachment.inactivePalette = style.inactivePalette;
    attachment.disabledPalette = style.disabledPalette;
    attachment.classHints = style.classHints;
    return attachment;
}

StyleTreeNode parseStyleTreeNodeValue(const QJsonObject& object)
{
    StyleTreeNode node;
    node.widget = parseWidgetInfo(object.value(QStringLiteral("widget")).toObject());
    node.style = parseStyleViewValue(object.value(QStringLiteral("style")).toObject());
    const QJsonArray children = object.value(QStringLiteral("children")).toArray();
    node.children.reserve(children.size());
    for (const QJsonValue& value : children) {
        node.children.push_back(std::make_shared<StyleTreeNode>(parseStyleTreeNodeValue(value.toObject())));
    }
    return node;
}

PageContext parsePageContextValue(const QJsonObject& object)
{
    PageContext context;
    if (object.contains(QStringLiteral("activeWindow"))) {
        context.activeWindow = parseSnapshotNode(object.value(QStringLiteral("activeWindow")).toObject());
        context.hasActiveWindow = true;
    }
    if (object.contains(QStringLiteral("focusWidget"))) {
        context.focusWidget = parseSnapshotNode(object.value(QStringLiteral("focusWidget")).toObject());
        context.hasFocusWidget = true;
    }
    if (object.contains(QStringLiteral("modalDialog"))) {
        context.modalDialog = parseSnapshotNode(object.value(QStringLiteral("modalDialog")).toObject());
        context.hasModalDialog = true;
    }
    return context;
}

} // namespace

StyleTreeView parseStyleTreeView(const QJsonObject& object)
{
    StyleTreeView view;
    view.includeChildren = object.value(QStringLiteral("includeChildren")).toBool();
    view.root = parseStyleTreeNodeValue(object.value(QStringLiteral("root")).toObject());
    return view;
}

ActivePageView parseActivePageView(const QJsonObject& object)
{
    ActivePageView view;
    view.page = parseSnapshotNode(object.value(QStringLiteral("page")).toObject());
    view.objectTree = parseSnapshotNode(object.value(QStringLiteral("objectTree")).toObject());
    view.layoutTree = parseLayoutNode(object.value(QStringLiteral("layoutTree")).toObject());
    return view;
}

WindowCapture parseWindowCapture(const QJsonObject& object)
{
    WindowCapture capture;
    capture.window = parseSnapshotNode(object.value(QStringLiteral("window")).toObject());
    const QByteArray bytes = QByteArray::fromBase64(object.value(QStringLiteral("imageBase64")).toString().toLatin1());
    if (!capture.image.loadFromData(bytes, "PNG")) {
        qWarning("parseWindowCapture: failed to decode PNG image from Base64 data");
    }
    return capture;
}

ActionResult parseActionResult(const QJsonObject& object)
{
    ActionResult result;
    if (object.contains(QStringLiteral("widget"))) {
        result.widget = parseSnapshotNode(object.value(QStringLiteral("widget")).toObject());
        result.hasWidget = true;
    }
    result.clickedObjectName = object.value(QStringLiteral("clicked")).toString();
    result.ref = object.value(QStringLiteral("ref")).toString();
    result.text = object.value(QStringLiteral("text")).toString();
    result.selectedText = object.value(QStringLiteral("selectedText")).toString();
    result.index = object.value(QStringLiteral("index")).toInt(-1);
    result.row = object.value(QStringLiteral("row")).toInt(-1);
    result.column = object.value(QStringLiteral("column")).toInt(-1);
    if (object.contains(QStringLiteral("checked"))) {
        result.checked = object.value(QStringLiteral("checked")).toBool();
        result.hasChecked = true;
    }
    result.widgetDestroyedAfterAction = object.value(QStringLiteral("widgetDestroyedAfterAction")).toBool();
    return result;
}

QVector<AssertionCheck> parseAssertionChecks(const QJsonArray& array)
{
    QVector<AssertionCheck> checks;
    checks.reserve(array.size());
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        checks.append(AssertionCheck{
            object.value(QStringLiteral("name")).toString(),
            parseVariant(object.value(QStringLiteral("expected"))),
            parseVariant(object.value(QStringLiteral("actual"))),
            object.value(QStringLiteral("passed")).toBool(),
        });
    }
    return checks;
}

WidgetCheckResult parseWidgetCheckResult(const QJsonObject& object)
{
    WidgetCheckResult result;
    result.widget = parseSnapshotNode(object.value(QStringLiteral("widget")).toObject());
    result.checks = parseAssertionChecks(object.value(QStringLiteral("checks")).toArray());
    result.elapsedMs = object.value(QStringLiteral("elapsedMs")).toInt(-1);
    return result;
}

LogEntry parseLogEntry(const QJsonObject& object)
{
    return LogEntry{
        object.value(QStringLiteral("level")).toString(),
        object.value(QStringLiteral("message")).toString(),
    };
}

QVector<LogEntry> parseLogEntries(const QJsonArray& array)
{
    QVector<LogEntry> entries;
    entries.reserve(array.size());
    for (const QJsonValue& value : array) {
        entries.append(parseLogEntry(value.toObject()));
    }
    return entries;
}

LogMatchResult parseLogMatchResult(const QJsonObject& object)
{
    LogMatchResult result;
    result.entry = parseLogEntry(object.value(QStringLiteral("entry")).toObject());
    result.elapsedMs = object.value(QStringLiteral("elapsedMs")).toInt(-1);
    result.entriesScanned = object.value(QStringLiteral("entriesScanned")).toInt();
    return result;
}

AutomationError parseAutomationError(const QJsonObject& response, const QString& transportError)
{
    AutomationError error;
    if (!transportError.isEmpty()) {
        error.code = ErrorCode::Transport;
        error.transportError = transportError;
        error.message = transportError;
        return error;
    }

    const QJsonObject errorObject = response.value(QStringLiteral("error")).toObject();
    error.bridgeCode = errorObject.value(QStringLiteral("code")).toString();
    error.code = errorCodeFromString(error.bridgeCode);
    error.message = errorObject.value(QStringLiteral("message")).toString();
    error.candidates = parseSnapshotNodes(errorObject.value(QStringLiteral("candidates")).toArray());
    error.timeoutMs = errorObject.value(QStringLiteral("timeoutMs")).toInt(-1);
    error.pollIntervalMs = errorObject.value(QStringLiteral("pollIntervalMs")).toInt(-1);
    error.limit = errorObject.value(QStringLiteral("limit")).toInt(-1);
    error.regexError = errorObject.value(QStringLiteral("regexError")).toString();

    if (errorObject.contains(QStringLiteral("widget")) || errorObject.contains(QStringLiteral("checks"))) {
        error.assertionReport = parseWidgetCheckResult(errorObject);
        error.hasAssertionReport = error.assertionReport.isValid();
    }

    const QJsonObject lastObservation = errorObject.value(QStringLiteral("lastObservation")).toObject();
    if (!lastObservation.isEmpty()) {
        WaitObservation observation;
        if (lastObservation.contains(QStringLiteral("widget"))) {
            observation.widget = parseSnapshotNode(lastObservation.value(QStringLiteral("widget")).toObject());
            observation.checks = parseAssertionChecks(lastObservation.value(QStringLiteral("checks")).toArray());
            observation.hasWidget = observation.widget.isValid();
        } else {
            observation.code = errorCodeFromString(lastObservation.value(QStringLiteral("code")).toString());
            observation.message = lastObservation.value(QStringLiteral("message")).toString();
            observation.candidates = parseSnapshotNodes(lastObservation.value(QStringLiteral("candidates")).toArray());
        }
        error.lastWidgetObservation = observation;
        error.hasLastWidgetObservation = observation.hasWidget || observation.code != ErrorCode::None ||
                                         !observation.message.isEmpty() || !observation.candidates.isEmpty();
    }

    return error;
}

TabChangedEvent parseTabChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload)
{
    TabChangedEvent event;
    event.subscription.id = subscriptionId;
    event.sequence = sequence;
    event.widget = parseSnapshotNode(payload.value(QStringLiteral("widget")).toObject());
    event.index = payload.value(QStringLiteral("index")).toInt(-1);
    event.previousIndex = payload.value(QStringLiteral("previousIndex")).toInt(-1);
    event.text = payload.value(QStringLiteral("text")).toString();
    event.previousText = payload.value(QStringLiteral("previousText")).toString();
    if (payload.contains(QStringLiteral("page"))) {
        event.page = parseSnapshotNode(payload.value(QStringLiteral("page")).toObject());
        event.hasPage = event.page.isValid();
    }
    if (payload.contains(QStringLiteral("previousPage"))) {
        event.previousPage = parseSnapshotNode(payload.value(QStringLiteral("previousPage")).toObject());
        event.hasPreviousPage = event.previousPage.isValid();
    }
    event.reason = payload.value(QStringLiteral("reason")).toString();
    return event;
}

ActivePageChangedEvent parseActivePageChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload)
{
    ActivePageChangedEvent event;
    event.subscription.id = subscriptionId;
    event.sequence = sequence;
    event.container = parseSnapshotNode(payload.value(QStringLiteral("container")).toObject());
    event.containerType = payload.value(QStringLiteral("containerType")).toString();
    event.index = payload.value(QStringLiteral("index")).toInt(-1);
    event.previousIndex = payload.value(QStringLiteral("previousIndex")).toInt(-1);
    if (payload.contains(QStringLiteral("page"))) {
        event.page = parseSnapshotNode(payload.value(QStringLiteral("page")).toObject());
        event.hasPage = event.page.isValid();
    }
    event.context = parsePageContextValue(payload.value(QStringLiteral("pageContext")).toObject());
    return event;
}

WindowFocusChangedEvent parseWindowFocusChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload)
{
    WindowFocusChangedEvent event;
    event.subscription.id = subscriptionId;
    event.sequence = sequence;
    if (payload.contains(QStringLiteral("window"))) {
        event.window = parseSnapshotNode(payload.value(QStringLiteral("window")).toObject());
        event.hasWindow = event.window.isValid();
    }
    if (payload.contains(QStringLiteral("previousWindow"))) {
        event.previousWindow = parseSnapshotNode(payload.value(QStringLiteral("previousWindow")).toObject());
        event.hasPreviousWindow = event.previousWindow.isValid();
    }
    event.windowTitle = payload.value(QStringLiteral("windowTitle")).toString();
    event.objectName = payload.value(QStringLiteral("objectName")).toString();
    return event;
}

FocusWidgetChangedEvent parseFocusWidgetChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload)
{
    FocusWidgetChangedEvent event;
    event.subscription.id = subscriptionId;
    event.sequence = sequence;
    if (payload.contains(QStringLiteral("widget"))) {
        event.widget = parseSnapshotNode(payload.value(QStringLiteral("widget")).toObject());
        event.hasWidget = event.widget.isValid();
    }
    if (payload.contains(QStringLiteral("previousWidget"))) {
        event.previousWidget = parseSnapshotNode(payload.value(QStringLiteral("previousWidget")).toObject());
        event.hasPreviousWidget = event.previousWidget.isValid();
    }
    if (payload.contains(QStringLiteral("window"))) {
        event.window = parseSnapshotNode(payload.value(QStringLiteral("window")).toObject());
        event.hasWindow = event.window.isValid();
    }
    event.focusReason = payload.value(QStringLiteral("focusReason")).toString();
    return event;
}

ModalDialogChangedEvent parseModalDialogChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload)
{
    ModalDialogChangedEvent event;
    event.subscription.id = subscriptionId;
    event.sequence = sequence;
    if (payload.contains(QStringLiteral("dialog"))) {
        event.dialog = parseSnapshotNode(payload.value(QStringLiteral("dialog")).toObject());
        event.hasDialog = event.dialog.isValid();
    }
    event.state = modalDialogStateFromString(payload.value(QStringLiteral("state")).toString());
    if (payload.contains(QStringLiteral("window"))) {
        event.window = parseSnapshotNode(payload.value(QStringLiteral("window")).toObject());
        event.hasWindow = event.window.isValid();
    }
    event.title = payload.value(QStringLiteral("title")).toString();
    return event;
}

} // namespace qtautotest::detail
