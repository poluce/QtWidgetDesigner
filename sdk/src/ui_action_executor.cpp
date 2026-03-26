#include "ui_action_executor.h"

#include "widget_introspection.h"

#include <qtautotest/action_observer.h>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QBuffer>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeySequence>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <QTextCursor>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWidget>

namespace {

QJsonObject errorObject(const QString& code, const QString& message, const QJsonArray& candidates = QJsonArray())
{
    QJsonObject object{
        {"ok", false},
        {"code", code},
        {"message", message},
    };

    if (!candidates.isEmpty()) {
        object.insert(QStringLiteral("candidates"), candidates);
    }

    return object;
}

bool prepareWidget(QWidget* widget, QString* errorMessage)
{
    if (widget == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Widget is null.");
        }
        return false;
    }

    if (!widget->isVisible()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Widget is not visible.");
        }
        return false;
    }

    if (!widget->isEnabled()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Widget is disabled.");
        }
        return false;
    }

    QWidget* window = widget->window();
    window->show();
    window->raise();
    window->activateWindow();
    QTest::qWait(50);

    return true;
}

QWidget* resolveWidget(const QJsonObject& selector, QString* errorMessage, QJsonArray* candidates = nullptr)
{
    return WidgetIntrospection::findSingleWidget(selector, errorMessage, candidates);
}

Qt::KeyboardModifiers parseModifiers(const QString& modifiers)
{
    Qt::KeyboardModifiers value = Qt::NoModifier;
    const QStringList parts = modifiers.split(QStringLiteral("+"), Qt::SkipEmptyParts);
    for (const QString& rawPart : parts) {
        const QString part = rawPart.trimmed().toLower();
        if (part == QStringLiteral("ctrl") || part == QStringLiteral("control")) {
            value |= Qt::ControlModifier;
        } else if (part == QStringLiteral("shift")) {
            value |= Qt::ShiftModifier;
        } else if (part == QStringLiteral("alt")) {
            value |= Qt::AltModifier;
        } else if (part == QStringLiteral("meta") || part == QStringLiteral("cmd") || part == QStringLiteral("win")) {
            value |= Qt::MetaModifier;
        }
    }

    return value;
}

Qt::Key parseKey(const QString& keyName)
{
    const QString key = keyName.trimmed();
    if (key.size() == 1) {
        const QChar ch = key.at(0).toUpper();
        return static_cast<Qt::Key>(ch.unicode());
    }

    static const QHash<QString, Qt::Key> keyMap{
        {QStringLiteral("Tab"), Qt::Key_Tab},
        {QStringLiteral("Enter"), Qt::Key_Return},
        {QStringLiteral("Return"), Qt::Key_Return},
        {QStringLiteral("Escape"), Qt::Key_Escape},
        {QStringLiteral("Esc"), Qt::Key_Escape},
        {QStringLiteral("Space"), Qt::Key_Space},
        {QStringLiteral("Backspace"), Qt::Key_Backspace},
        {QStringLiteral("Delete"), Qt::Key_Delete},
        {QStringLiteral("Insert"), Qt::Key_Insert},
        {QStringLiteral("Up"), Qt::Key_Up},
        {QStringLiteral("Down"), Qt::Key_Down},
        {QStringLiteral("Left"), Qt::Key_Left},
        {QStringLiteral("Right"), Qt::Key_Right},
        {QStringLiteral("Home"), Qt::Key_Home},
        {QStringLiteral("End"), Qt::Key_End},
        {QStringLiteral("PageUp"), Qt::Key_PageUp},
        {QStringLiteral("PageDown"), Qt::Key_PageDown},
        {QStringLiteral("F1"), Qt::Key_F1},
        {QStringLiteral("F2"), Qt::Key_F2},
        {QStringLiteral("F3"), Qt::Key_F3},
        {QStringLiteral("F4"), Qt::Key_F4},
        {QStringLiteral("F5"), Qt::Key_F5},
        {QStringLiteral("F6"), Qt::Key_F6},
        {QStringLiteral("F7"), Qt::Key_F7},
        {QStringLiteral("F8"), Qt::Key_F8},
        {QStringLiteral("F9"), Qt::Key_F9},
        {QStringLiteral("F10"), Qt::Key_F10},
        {QStringLiteral("F11"), Qt::Key_F11},
        {QStringLiteral("F12"), Qt::Key_F12},
    };

    return keyMap.value(key, Qt::Key_unknown);
}

QTreeWidgetItem* findTreeItem(QTreeWidget* tree, const QJsonArray& path)
{
    if (tree == nullptr || path.isEmpty()) {
        return nullptr;
    }

    QTreeWidgetItem* current = nullptr;
    for (int depth = 0; depth < path.size(); ++depth) {
        const QString expected = path.at(depth).toString();
        QTreeWidgetItem* next = nullptr;

        const int childCount = current == nullptr ? tree->topLevelItemCount() : current->childCount();
        for (int i = 0; i < childCount; ++i) {
            QTreeWidgetItem* item = current == nullptr ? tree->topLevelItem(i) : current->child(i);
            if (item->text(0) == expected) {
                next = item;
                break;
            }
        }

        if (next == nullptr) {
            return nullptr;
        }
        current = next;
    }

    return current;
}

qtautotest::ActionObserver* currentActionObserver()
{
    qtautotest::ActionObserver* observer = qtautotest::actionObserver();
    return observer != nullptr && observer->isEnabled() ? observer : nullptr;
}

} // namespace

namespace UiActionExecutor {

QJsonObject click(const QJsonObject& selector)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = WidgetIntrospection::findSingleWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(
            candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
            errorMessage,
            candidates);
    }

    if (!prepareWidget(widget, &errorMessage)) {
        return errorObject(QStringLiteral("not_clickable"), errorMessage);
    }

    const QString widgetRef = WidgetIntrospection::refForWidget(widget);
    const QString widgetObjectName = widget->objectName();
    QPointer<QWidget> safeWidget(widget);

    const QPoint clickPoint = widget->rect().center();
    if (qtautotest::ActionObserver* observer = currentActionObserver()) {
        observer->prepareForClick(widget, clickPoint);
    }

    if (qtautotest::ActionObserver* observer = currentActionObserver()) {
        QTest::mouseMove(widget, clickPoint);
        QTest::mousePress(widget, Qt::LeftButton, Qt::NoModifier, clickPoint);
        observer->showClickPulse(widget, clickPoint);
        QTest::qWait(observer->mousePressHoldMs());
        QTest::mouseRelease(widget, Qt::LeftButton, Qt::NoModifier, clickPoint);
        observer->finishAction();
    } else {
        QTest::mouseMove(widget, clickPoint);
        QTest::mouseClick(widget, Qt::LeftButton, Qt::NoModifier, clickPoint);
        QTest::qWait(30);
    }

    QJsonObject result{
        {"ok", true},
        {"clicked", widgetObjectName},
        {"ref", widgetRef},
    };

    if (safeWidget != nullptr) {
        result.insert(QStringLiteral("widget"), WidgetIntrospection::widgetSummary(safeWidget));
    } else {
        result.insert(QStringLiteral("widgetDestroyedAfterAction"), true);
    }

    return result;
}

QJsonObject setText(const QJsonObject& selector, const QString& text)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = WidgetIntrospection::findSingleWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(
            candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
            errorMessage,
            candidates);
    }

    if (!prepareWidget(widget, &errorMessage)) {
        return errorObject(QStringLiteral("not_editable"), errorMessage);
    }

    if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        if (lineEdit->isReadOnly()) {
            return errorObject(QStringLiteral("unsupported_widget"), QStringLiteral("Line edit is read-only."));
        }
        if (qtautotest::ActionObserver* observer = currentActionObserver()) {
            observer->prepareForTyping(widget);
        }
        lineEdit->setFocus();
        lineEdit->selectAll();
        QTest::keyClick(lineEdit, Qt::Key_Backspace);
        QTest::keyClicks(lineEdit, text, Qt::NoModifier,
                         currentActionObserver() != nullptr ? currentActionObserver()->keyDelayMs() : 0);
    } else if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(widget)) {
        if (plainTextEdit->isReadOnly()) {
            return errorObject(QStringLiteral("unsupported_widget"), QStringLiteral("Plain text edit is read-only."));
        }
        if (qtautotest::ActionObserver* observer = currentActionObserver()) {
            observer->prepareForTyping(widget);
        }
        plainTextEdit->setFocus();
        plainTextEdit->selectAll();
        QTest::keyClick(plainTextEdit, Qt::Key_Backspace);
        QTest::keyClicks(plainTextEdit, text, Qt::NoModifier,
                         currentActionObserver() != nullptr ? currentActionObserver()->keyDelayMs() : 0);
    } else if (auto* textEdit = qobject_cast<QTextEdit*>(widget)) {
        if (textEdit->isReadOnly()) {
            return errorObject(QStringLiteral("unsupported_widget"), QStringLiteral("Text edit is read-only."));
        }
        if (qtautotest::ActionObserver* observer = currentActionObserver()) {
            observer->prepareForTyping(widget);
        }
        textEdit->setFocus();
        textEdit->selectAll();
        textEdit->textCursor().removeSelectedText();
        QTest::keyClicks(textEdit, text, Qt::NoModifier,
                         currentActionObserver() != nullptr ? currentActionObserver()->keyDelayMs() : 0);
    } else {
        return errorObject(QStringLiteral("unsupported_widget"), QStringLiteral("Widget does not support set_text."));
    }

    if (qtautotest::ActionObserver* observer = currentActionObserver()) {
        observer->finishAction();
    } else {
        QTest::qWait(30);
    }

    return QJsonObject{
        {"ok", true},
        {"text", text},
        {"widget", WidgetIntrospection::widgetSummary(widget)},
    };
}

QJsonObject captureWindow(const QJsonObject& selector)
{
    QString errorMessage;
    QWidget* window = WidgetIntrospection::chooseWindowTarget(selector, &errorMessage);
    if (window == nullptr) {
        return errorObject(QStringLiteral("window_not_found"), errorMessage);
    }

    window->show();
    window->raise();
    window->activateWindow();
    QTest::qWait(50);

    QPixmap pixmap(window->size());
    window->render(&pixmap);

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");

    return QJsonObject{
        {"ok", true},
        {"window", WidgetIntrospection::widgetSummary(window)},
        {"imageBase64", QString::fromLatin1(pngBytes.toBase64())},
    };
}

QJsonObject focusWindow(const QJsonObject& selector)
{
    QString errorMessage;
    QWidget* window = WidgetIntrospection::chooseWindowTarget(selector, &errorMessage);
    if (window == nullptr) {
        return errorObject(QStringLiteral("window_not_found"), errorMessage);
    }

    window->show();
    window->raise();
    window->activateWindow();
    window->setFocus();
    QTest::qWait(50);

    return QJsonObject{
        {"ok", true},
        {"window", WidgetIntrospection::widgetSummary(window)},
    };
}

QJsonObject pressKey(const QJsonObject& selector, const QString& key, const QString& modifiers)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = selector.isEmpty() ? QApplication::focusWidget() : resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage.isEmpty() ? QStringLiteral("No target widget is available.") : errorMessage,
                           candidates);
    }

    if (!prepareWidget(widget, &errorMessage)) {
        return errorObject(QStringLiteral("not_focusable"), errorMessage);
    }

    const Qt::Key qtKey = parseKey(key);
    if (qtKey == Qt::Key_unknown) {
        return errorObject(QStringLiteral("unknown_key"), QStringLiteral("Unsupported key name."));
    }

    widget->setFocus();
    QTest::keyClick(widget, qtKey, parseModifiers(modifiers));
    QTest::qWait(30);

    return QJsonObject{
        {"ok", true},
        {"key", key},
        {"widget", WidgetIntrospection::widgetSummary(widget)},
    };
}

QJsonObject sendShortcut(const QJsonObject& selector, const QString& shortcut)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = selector.isEmpty() ? QApplication::focusWidget() : resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage.isEmpty() ? QStringLiteral("No target widget is available.") : errorMessage,
                           candidates);
    }

    if (!prepareWidget(widget, &errorMessage)) {
        return errorObject(QStringLiteral("not_focusable"), errorMessage);
    }

    const QKeySequence sequence(shortcut);
    if (sequence.count() == 0) {
        return errorObject(QStringLiteral("invalid_shortcut"), QStringLiteral("Shortcut could not be parsed."));
    }

    widget->setFocus();
    QTest::keySequence(widget, sequence);
    QTest::qWait(30);

    return QJsonObject{
        {"ok", true},
        {"shortcut", shortcut},
        {"widget", WidgetIntrospection::widgetSummary(widget)},
    };
}

QJsonObject scroll(const QJsonObject& selector, const QString& direction, int amount)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    QWidget* scrollTarget = qobject_cast<QAbstractScrollArea*>(widget) != nullptr || qobject_cast<QScrollArea*>(widget) != nullptr
        ? widget
        : WidgetIntrospection::scrollContainerForWidget(widget);
    if (scrollTarget == nullptr) {
        return errorObject(QStringLiteral("not_scrollable"), QStringLiteral("No scroll container was found."));
    }

    auto* area = qobject_cast<QAbstractScrollArea*>(scrollTarget);
    if (area == nullptr) {
        return errorObject(QStringLiteral("not_scrollable"), QStringLiteral("Target does not support scrolling."));
    }

    QScrollBar* bar = (direction == QStringLiteral("left") || direction == QStringLiteral("right"))
        ? area->horizontalScrollBar()
        : area->verticalScrollBar();
    if (bar == nullptr) {
        return errorObject(QStringLiteral("not_scrollable"), QStringLiteral("Scroll bar is not available."));
    }

    const int safeAmount = amount == 0 ? 120 : amount;
    int delta = safeAmount;
    if (direction == QStringLiteral("up") || direction == QStringLiteral("left")) {
        delta = -safeAmount;
    }
    bar->setValue(bar->value() + delta);
    QTest::qWait(30);

    return QJsonObject{
        {"ok", true},
        {"direction", direction},
        {"value", bar->value()},
        {"widget", WidgetIntrospection::widgetSummary(scrollTarget)},
    };
}

QJsonObject scrollIntoView(const QJsonObject& selector)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    QWidget* scrollTarget = WidgetIntrospection::scrollContainerForWidget(widget);
    if (auto* scrollArea = qobject_cast<QScrollArea*>(scrollTarget)) {
        scrollArea->ensureWidgetVisible(widget, 24, 24);
        QTest::qWait(30);
        return QJsonObject{
            {"ok", true},
            {"widget", WidgetIntrospection::widgetSummary(widget)},
            {"scrollContainer", WidgetIntrospection::widgetSummary(scrollArea)},
        };
    }

    return errorObject(QStringLiteral("not_scrollable"), QStringLiteral("No compatible scroll area found."));
}

QJsonObject selectItem(const QJsonObject& selector, const QJsonObject& options)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    if (auto* listWidget = qobject_cast<QListWidget*>(widget)) {
        QListWidgetItem* item = nullptr;
        if (options.contains(QStringLiteral("row"))) {
            item = listWidget->item(options.value(QStringLiteral("row")).toInt(-1));
        } else {
            const QString text = options.value(QStringLiteral("text")).toString();
            const auto items = listWidget->findItems(text, Qt::MatchExactly);
            if (!items.isEmpty()) {
                item = items.first();
            }
        }
        if (item == nullptr) {
            return errorObject(QStringLiteral("not_selectable"), QStringLiteral("List item was not found."));
        }
        listWidget->setCurrentItem(item);
        QTest::qWait(30);
        return QJsonObject{{"ok", true}, {"selectedText", item->text()}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
    }

    if (auto* treeWidget = qobject_cast<QTreeWidget*>(widget)) {
        QTreeWidgetItem* item = findTreeItem(treeWidget, options.value(QStringLiteral("path")).toArray());
        if (item == nullptr && options.contains(QStringLiteral("text"))) {
            const QList<QTreeWidgetItem*> items = treeWidget->findItems(options.value(QStringLiteral("text")).toString(),
                                                                        Qt::MatchRecursive | Qt::MatchExactly);
            if (!items.isEmpty()) {
                item = items.first();
            }
        }
        if (item == nullptr) {
            return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Tree item was not found."));
        }
        treeWidget->setCurrentItem(item);
        QTest::qWait(30);
        return QJsonObject{{"ok", true}, {"selectedText", item->text(0)}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
    }

    if (auto* tableWidget = qobject_cast<QTableWidget*>(widget)) {
        const int row = options.value(QStringLiteral("row")).toInt(-1);
        const int column = options.value(QStringLiteral("column")).toInt(-1);
        if (row < 0 || column < 0 || row >= tableWidget->rowCount() || column >= tableWidget->columnCount()) {
            return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Table cell was not found."));
        }
        tableWidget->setCurrentCell(row, column);
        const QString text = tableWidget->item(row, column) != nullptr ? tableWidget->item(row, column)->text() : QString();
        QTest::qWait(30);
        return QJsonObject{{"ok", true}, {"selectedText", text}, {"row", row}, {"column", column}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
    }

    return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Widget does not support item selection."));
}

QJsonObject toggleCheck(const QJsonObject& selector, bool checked)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    auto* button = qobject_cast<QAbstractButton*>(widget);
    if (button == nullptr || !button->isCheckable()) {
        return errorObject(QStringLiteral("not_checkable"), QStringLiteral("Widget is not checkable."));
    }

    if (button->isChecked() != checked) {
        const QJsonObject clickResult = click(selector);
        if (!clickResult.value(QStringLiteral("ok")).toBool()) {
            return clickResult;
        }
    }

    return QJsonObject{{"ok", true}, {"checked", button->isChecked()}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
}

QJsonObject chooseComboOption(const QJsonObject& selector, const QString& text, int index)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    auto* comboBox = qobject_cast<QComboBox*>(widget);
    if (comboBox == nullptr) {
        return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Widget is not a combo box."));
    }

    int targetIndex = index;
    if (targetIndex < 0 && !text.isEmpty()) {
        targetIndex = comboBox->findText(text, Qt::MatchExactly);
    }
    if (targetIndex < 0 || targetIndex >= comboBox->count()) {
        return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Combo option was not found."));
    }

    comboBox->setCurrentIndex(targetIndex);
    QTest::qWait(30);

    return QJsonObject{{"ok", true}, {"index", targetIndex}, {"text", comboBox->currentText()}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
}

QJsonObject activateTab(const QJsonObject& selector, const QString& tabText, int index)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    int targetIndex = index;

    if (auto* tabWidget = qobject_cast<QTabWidget*>(widget)) {
        if (targetIndex < 0 && !tabText.isEmpty()) {
            for (int i = 0; i < tabWidget->count(); ++i) {
                if (tabWidget->tabText(i) == tabText) {
                    targetIndex = i;
                    break;
                }
            }
        }
        if (targetIndex < 0 || targetIndex >= tabWidget->count()) {
            return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Tab was not found."));
        }
        tabWidget->setCurrentIndex(targetIndex);
        QTest::qWait(30);
        return QJsonObject{{"ok", true}, {"index", targetIndex}, {"text", tabWidget->tabText(targetIndex)}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
    }

    if (auto* tabBar = qobject_cast<QTabBar*>(widget)) {
        if (targetIndex < 0 && !tabText.isEmpty()) {
            for (int i = 0; i < tabBar->count(); ++i) {
                if (tabBar->tabText(i) == tabText) {
                    targetIndex = i;
                    break;
                }
            }
        }
        if (targetIndex < 0 || targetIndex >= tabBar->count()) {
            return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Tab was not found."));
        }
        tabBar->setCurrentIndex(targetIndex);
        QTest::qWait(30);
        return QJsonObject{{"ok", true}, {"index", targetIndex}, {"text", tabBar->tabText(targetIndex)}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
    }

    return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Widget is not a tab container."));
}

QJsonObject switchStackedPage(const QJsonObject& selector, int index)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    auto* stacked = qobject_cast<QStackedWidget*>(widget);
    if (stacked == nullptr) {
        return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Widget is not a stacked widget."));
    }
    if (index < 0 || index >= stacked->count()) {
        return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Stacked page index is out of range."));
    }

    stacked->setCurrentIndex(index);
    QTest::qWait(30);
    return QJsonObject{{"ok", true}, {"index", index}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
}

QJsonObject expandTreeNode(const QJsonObject& selector, const QJsonArray& path)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    auto* tree = qobject_cast<QTreeWidget*>(widget);
    if (tree == nullptr) {
        return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Widget is not a tree widget."));
    }

    QTreeWidgetItem* item = findTreeItem(tree, path);
    if (item == nullptr) {
        return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Tree node was not found."));
    }

    tree->expandItem(item);
    QTest::qWait(30);
    return QJsonObject{{"ok", true}, {"text", item->text(0)}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
}

QJsonObject collapseTreeNode(const QJsonObject& selector, const QJsonArray& path)
{
    QString errorMessage;
    QJsonArray candidates;
    QWidget* widget = resolveWidget(selector, &errorMessage, &candidates);
    if (widget == nullptr) {
        return errorObject(candidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector"),
                           errorMessage, candidates);
    }

    auto* tree = qobject_cast<QTreeWidget*>(widget);
    if (tree == nullptr) {
        return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Widget is not a tree widget."));
    }

    QTreeWidgetItem* item = findTreeItem(tree, path);
    if (item == nullptr) {
        return errorObject(QStringLiteral("not_selectable"), QStringLiteral("Tree node was not found."));
    }

    tree->collapseItem(item);
    QTest::qWait(30);
    return QJsonObject{{"ok", true}, {"text", item->text(0)}, {"widget", WidgetIntrospection::widgetSummary(widget)}};
}

} // namespace UiActionExecutor
