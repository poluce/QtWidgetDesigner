#pragma once

#include <QJsonArray>
#include <QJsonObject>

class QWidget;

namespace WidgetIntrospection {

QJsonObject describeUi();
QJsonObject describeSnapshot();
QJsonObject describeObjectTree(bool visibleOnly = false);
QJsonObject describeLayoutTree(bool visibleOnly = false);
QJsonObject describeSubtree(const QJsonObject& selector, bool layoutTree, bool visibleOnly,
                            QString* errorCode = nullptr, QString* errorMessage = nullptr,
                            QJsonArray* candidates = nullptr);
QJsonObject describeStyle(const QJsonObject& selector, bool includeChildren = false,
                          QString* errorCode = nullptr, QString* errorMessage = nullptr,
                          QJsonArray* candidates = nullptr);
QJsonObject describeActivePage();
QJsonArray listWindows();
QJsonArray findWidgets(const QJsonObject& selector);
QWidget* findSingleWidget(const QJsonObject& selector, QString* errorMessage, QJsonArray* candidates = nullptr);
QWidget* chooseWindowTarget(const QJsonObject& selector, QString* errorMessage);
QJsonObject widgetSummary(const QWidget* widget);
QJsonObject styleSummary(const QWidget* widget);
QJsonObject layoutSummary(const QWidget* widget);
QString refForWidget(const QWidget* widget);
QWidget* widgetForRef(const QString& ref);
QWidget* activePageWidget();
bool isOnCurrentPage(const QWidget* widget);
int indexInParent(const QWidget* widget);
QWidget* scrollContainerForWidget(const QWidget* widget);

} // namespace WidgetIntrospection
