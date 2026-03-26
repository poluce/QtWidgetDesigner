#include "bridge_operations.h"

#include "app_log_sink.h"
#include "ui_action_executor.h"
#include "widget_introspection.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTest>

namespace {

QJsonObject errorObject(const QString& code, const QString& message, const QJsonObject& details = QJsonObject())
{
    QJsonObject object{
        {"ok", false},
        {"code", code},
        {"message", message},
    };

    for (auto it = details.begin(); it != details.end(); ++it) {
        if (it.key() == QStringLiteral("ok") ||
            it.key() == QStringLiteral("code") ||
            it.key() == QStringLiteral("message")) {
            continue;
        }
        object.insert(it.key(), it.value());
    }

    return object;
}

QJsonObject successObject(const QJsonObject& payload)
{
    QJsonObject object = payload;
    object.insert(QStringLiteral("ok"), true);
    return object;
}

QJsonObject summaryForWidget(QWidget* widget)
{
    return WidgetIntrospection::widgetSummary(widget);
}

QJsonObject actualValues(const QJsonObject& summary)
{
    return QJsonObject{
        {"ref", summary.value(QStringLiteral("ref"))},
        {"objectName", summary.value(QStringLiteral("objectName"))},
        {"className", summary.value(QStringLiteral("className"))},
        {"text", summary.value(QStringLiteral("text"))},
        {"placeholderText", summary.value(QStringLiteral("placeholderText"))},
        {"windowTitle", summary.value(QStringLiteral("windowTitle"))},
        {"visible", summary.value(QStringLiteral("visible"))},
        {"enabled", summary.value(QStringLiteral("enabled"))},
        {"isInteractive", summary.value(QStringLiteral("isInteractive"))},
        {"isInput", summary.value(QStringLiteral("isInput"))},
        {"isCurrentPage", summary.value(QStringLiteral("isCurrentPage"))},
        {"path", summary.value(QStringLiteral("path"))},
        {"riskLevel", summary.value(QStringLiteral("riskLevel"))},
    };
}

QJsonArray evaluateAssertions(const QJsonObject& summary, const QJsonObject& assertions, bool* allPassed)
{
    QJsonArray checks;
    bool passed = true;

    const auto addCheck = [&](const QString& name, const QJsonValue& expected, const QJsonValue& actual, bool checkPassed) {
        checks.append(QJsonObject{
            {"name", name},
            {"expected", expected},
            {"actual", actual},
            {"passed", checkPassed},
        });
        passed = passed && checkPassed;
    };

    if (assertions.isEmpty()) {
        addCheck(QStringLiteral("exists"), true, true, true);
    }

    if (assertions.contains(QStringLiteral("textEquals"))) {
        const QString expected = assertions.value(QStringLiteral("textEquals")).toString();
        const QString actual = summary.value(QStringLiteral("text")).toString();
        addCheck(QStringLiteral("textEquals"), expected, actual, actual == expected);
    }

    if (assertions.contains(QStringLiteral("textContains"))) {
        const QString expected = assertions.value(QStringLiteral("textContains")).toString();
        const QString actual = summary.value(QStringLiteral("text")).toString();
        addCheck(QStringLiteral("textContains"), expected, actual,
                 actual.contains(expected, Qt::CaseInsensitive));
    }

    if (assertions.contains(QStringLiteral("visible"))) {
        const bool expected = assertions.value(QStringLiteral("visible")).toBool();
        const bool actual = summary.value(QStringLiteral("visible")).toBool();
        addCheck(QStringLiteral("visible"), expected, actual, actual == expected);
    }

    if (assertions.contains(QStringLiteral("enabled"))) {
        const bool expected = assertions.value(QStringLiteral("enabled")).toBool();
        const bool actual = summary.value(QStringLiteral("enabled")).toBool();
        addCheck(QStringLiteral("enabled"), expected, actual, actual == expected);
    }

    if (assertions.contains(QStringLiteral("objectName"))) {
        const QString expected = assertions.value(QStringLiteral("objectName")).toString();
        const QString actual = summary.value(QStringLiteral("objectName")).toString();
        addCheck(QStringLiteral("objectName"), expected, actual, actual == expected);
    }

    if (assertions.contains(QStringLiteral("className"))) {
        const QString expected = assertions.value(QStringLiteral("className")).toString();
        const QString actual = summary.value(QStringLiteral("className")).toString();
        addCheck(QStringLiteral("className"), expected, actual, actual == expected);
    }

    if (assertions.contains(QStringLiteral("windowTitleContains"))) {
        const QString expected = assertions.value(QStringLiteral("windowTitleContains")).toString();
        const QString actual = summary.value(QStringLiteral("windowTitle")).toString();
        addCheck(QStringLiteral("windowTitleContains"), expected, actual,
                 actual.contains(expected, Qt::CaseInsensitive));
    }

    if (assertions.contains(QStringLiteral("isInteractive"))) {
        const bool expected = assertions.value(QStringLiteral("isInteractive")).toBool();
        const bool actual = summary.value(QStringLiteral("isInteractive")).toBool();
        addCheck(QStringLiteral("isInteractive"), expected, actual, actual == expected);
    }

    if (assertions.contains(QStringLiteral("isInput"))) {
        const bool expected = assertions.value(QStringLiteral("isInput")).toBool();
        const bool actual = summary.value(QStringLiteral("isInput")).toBool();
        addCheck(QStringLiteral("isInput"), expected, actual, actual == expected);
    }

    if (assertions.contains(QStringLiteral("riskLevel"))) {
        const QString expected = assertions.value(QStringLiteral("riskLevel")).toString();
        const QString actual = summary.value(QStringLiteral("riskLevel")).toString();
        addCheck(QStringLiteral("riskLevel"), expected, actual, actual == expected);
    }

    if (allPassed != nullptr) {
        *allPassed = passed;
    }

    return checks;
}

bool logMatches(const QJsonObject& entry, const QString& textContains, const QRegularExpression& regex)
{
    const QString message = entry.value(QStringLiteral("message")).toString();

    if (!textContains.isEmpty() && !message.contains(textContains, Qt::CaseInsensitive)) {
        return false;
    }

    if (regex.isValid() && !regex.pattern().isEmpty()) {
        const QRegularExpressionMatch match = regex.match(message);
        if (!match.hasMatch()) {
            return false;
        }
    }

    return true;
}

} // namespace

namespace BridgeOperations {

QJsonObject describeSnapshot()
{
    return successObject(WidgetIntrospection::describeSnapshot());
}

QJsonObject describeObjectTree(bool visibleOnly)
{
    return successObject(WidgetIntrospection::describeObjectTree(visibleOnly));
}

QJsonObject describeLayoutTree(bool visibleOnly)
{
    return successObject(WidgetIntrospection::describeLayoutTree(visibleOnly));
}

QJsonObject describeSubtree(const QJsonObject& selector, bool layoutTree, bool visibleOnly)
{
    QString errorCode;
    QString errorMessage;
    QJsonArray candidates;
    const QJsonObject subtree = WidgetIntrospection::describeSubtree(selector, layoutTree, visibleOnly,
                                                                     &errorCode, &errorMessage, &candidates);
    if (subtree.isEmpty()) {
        return errorObject(errorCode, errorMessage, QJsonObject{{"candidates", candidates}});
    }

    return successObject(QJsonObject{
        {"treeType", layoutTree ? QStringLiteral("layout") : QStringLiteral("object")},
        {"visibleOnly", visibleOnly},
        {"root", subtree},
    });
}

QJsonObject describeActivePage()
{
    const QJsonObject page = WidgetIntrospection::describeActivePage();
    if (page.isEmpty()) {
        return errorObject(QStringLiteral("not_found"), QStringLiteral("No active page is available."));
    }

    return successObject(page);
}

QJsonObject listWindows()
{
    return successObject(QJsonObject{
        {"windows", WidgetIntrospection::listWindows()},
    });
}

QJsonObject focusWindow(const QJsonObject& selector)
{
    return UiActionExecutor::focusWindow(selector);
}

QJsonObject assertWidget(const QJsonObject& selector, const QJsonObject& assertions)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = WidgetIntrospection::findSingleWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(
            candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
            errorMessage,
            QJsonObject{{"candidates", candidates}});
    }

    const QJsonObject summary = summaryForWidget(widget);
    bool passed = false;
    const QJsonArray checks = evaluateAssertions(summary, assertions, &passed);
    if (!passed) {
        return errorObject(QStringLiteral("assertion_failed"),
                           QStringLiteral("Widget assertions failed."),
                           QJsonObject{
                               {"widget", summary},
                               {"checks", checks},
                               {"actual", actualValues(summary)},
                           });
    }

    return successObject(QJsonObject{
        {"widget", summary},
        {"checks", checks},
        {"actual", actualValues(summary)},
    });
}

QJsonObject waitForWidget(const QJsonObject& selector, const QJsonObject& assertions,
                          int timeoutMs, int pollIntervalMs)
{
    const int safeTimeoutMs = timeoutMs > 0 ? timeoutMs : 3000;
    const int safePollIntervalMs = pollIntervalMs > 0 ? pollIntervalMs : 100;

    QElapsedTimer timer;
    timer.start();

    QJsonObject lastObservation;

    while (timer.elapsed() <= safeTimeoutMs) {
        QString errorMessage;
        QJsonArray candidates;
        QWidget* widget = WidgetIntrospection::findSingleWidget(selector, &errorMessage, &candidates);

        if (widget != nullptr) {
            const QJsonObject summary = summaryForWidget(widget);
            bool passed = false;
            const QJsonArray checks = evaluateAssertions(summary, assertions, &passed);

            lastObservation = QJsonObject{
                {"widget", summary},
                {"checks", checks},
                {"actual", actualValues(summary)},
            };

            if (passed) {
                return successObject(QJsonObject{
                    {"elapsedMs", timer.elapsed()},
                    {"widget", summary},
                    {"checks", checks},
                    {"actual", actualValues(summary)},
                });
            }
        } else {
            lastObservation = QJsonObject{
                {"code", candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector")},
                {"message", errorMessage},
                {"candidates", candidates},
            };
        }

        QCoreApplication::processEvents();
        QTest::qWait(safePollIntervalMs);
    }

    return errorObject(QStringLiteral("timeout"),
                       QStringLiteral("Timed out waiting for widget."),
                       QJsonObject{
                           {"timeoutMs", safeTimeoutMs},
                           {"pollIntervalMs", safePollIntervalMs},
                           {"lastObservation", lastObservation},
                       });
}

QJsonObject waitForLog(const QString& textContains, const QString& regex,
                       int timeoutMs, int pollIntervalMs, int limit)
{
    if (textContains.isEmpty() && regex.isEmpty()) {
        return errorObject(QStringLiteral("missing_expectation"),
                           QStringLiteral("wait_for_log requires textContains or regex."));
    }

    const QRegularExpression expression(regex);
    if (!regex.isEmpty() && !expression.isValid()) {
        return errorObject(QStringLiteral("invalid_regex"),
                           QStringLiteral("Invalid regular expression."),
                           QJsonObject{{"regexError", expression.errorString()}});
    }

    const int safeTimeoutMs = timeoutMs > 0 ? timeoutMs : 3000;
    const int safePollIntervalMs = pollIntervalMs > 0 ? pollIntervalMs : 100;
    const int safeLimit = limit > 0 ? limit : 200;

    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() <= safeTimeoutMs) {
        const QJsonArray entries = AppLogSink::instance().recentEntries(safeLimit);
        for (const QJsonValue& value : entries) {
            const QJsonObject entry = value.toObject();
            if (logMatches(entry, textContains, expression)) {
                return successObject(QJsonObject{
                    {"elapsedMs", timer.elapsed()},
                    {"entry", entry},
                    {"entriesScanned", entries.size()},
                });
            }
        }

        QCoreApplication::processEvents();
        QTest::qWait(safePollIntervalMs);
    }

    return errorObject(QStringLiteral("timeout"),
                       QStringLiteral("Timed out waiting for log entry."),
                       QJsonObject{
                           {"timeoutMs", safeTimeoutMs},
                           {"pollIntervalMs", safePollIntervalMs},
                           {"limit", safeLimit},
                       });
}

} // namespace BridgeOperations
