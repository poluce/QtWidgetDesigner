#pragma once

#include <QJsonObject>

namespace BridgeOperations {

QJsonObject describeSnapshot();
QJsonObject describeObjectTree(bool visibleOnly);
QJsonObject describeLayoutTree(bool visibleOnly);
QJsonObject describeSubtree(const QJsonObject& selector, bool layoutTree, bool visibleOnly);
QJsonObject describeActivePage();
QJsonObject listWindows();
QJsonObject focusWindow(const QJsonObject& selector);
QJsonObject assertWidget(const QJsonObject& selector, const QJsonObject& assertions);
QJsonObject waitForWidget(const QJsonObject& selector, const QJsonObject& assertions,
                          int timeoutMs, int pollIntervalMs);
QJsonObject waitForLog(const QString& textContains, const QString& regex,
                       int timeoutMs, int pollIntervalMs, int limit);

} // namespace BridgeOperations
