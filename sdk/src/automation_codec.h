#pragma once

#include <qtautotest/automation_client.h>
#include <qtautotest/automation_event_client.h>

#include <QJsonArray>
#include <QJsonObject>

namespace qtautotest::detail {

ErrorCode errorCodeFromString(const QString& code);
CommandKind commandKindFromString(const QString& command);
EventKind eventKindFromString(const QString& event);
ModalDialogState modalDialogStateFromString(const QString& state);

QJsonObject encodeSelector(const Selector& selector);
QJsonArray encodeSelectors(const QVector<Selector>& selectors);
QJsonObject encodeWidgetAssertions(const WidgetAssertions& assertions);
QJsonObject encodeListItemTarget(const ListItemTarget& target);
QJsonObject encodeTreeItemTarget(const TreeItemTarget& target);
QJsonArray encodeTreePath(const QStringList& path);
QString encodeScrollDirection(ScrollDirection direction);
QString encodeKey(Qt::Key key);
QString encodeModifiers(Qt::KeyboardModifiers modifiers);

WidgetInfo parseWidgetInfo(const QJsonObject& object);
SnapshotNode parseSnapshotNode(const QJsonObject& object);
QVector<SnapshotNode> parseSnapshotNodes(const QJsonArray& array);
WindowInfo parseWindowInfo(const QJsonObject& object);
QVector<WindowInfo> parseWindowInfos(const QJsonArray& array);
LayoutNode parseLayoutNode(const QJsonObject& object);
SnapshotView parseSnapshotView(const QJsonObject& object);
ObjectTreeView parseObjectTreeView(const QJsonObject& object);
LayoutTreeView parseLayoutTreeView(const QJsonObject& object);
SubtreeView parseSubtreeView(const QJsonObject& object);
StyleTreeView parseStyleTreeView(const QJsonObject& object);
ActivePageView parseActivePageView(const QJsonObject& object);
WindowCapture parseWindowCapture(const QJsonObject& object);
ActionResult parseActionResult(const QJsonObject& object);
QVector<AssertionCheck> parseAssertionChecks(const QJsonArray& array);
WidgetCheckResult parseWidgetCheckResult(const QJsonObject& object);
LogEntry parseLogEntry(const QJsonObject& object);
QVector<LogEntry> parseLogEntries(const QJsonArray& array);
LogMatchResult parseLogMatchResult(const QJsonObject& object);
AutomationError parseAutomationError(const QJsonObject& response, const QString& transportError = QString());

TabChangedEvent parseTabChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload);
ActivePageChangedEvent parseActivePageChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload);
WindowFocusChangedEvent parseWindowFocusChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload);
FocusWidgetChangedEvent parseFocusWidgetChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload);
ModalDialogChangedEvent parseModalDialogChangedEvent(const QString& subscriptionId, qint64 sequence, const QJsonObject& payload);

} // namespace qtautotest::detail
