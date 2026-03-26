#include "widget_introspection.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMutex>
#include <QMutexLocker>
#include <QListWidget>
#include <QMainWindow>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolBox>
#include <QTreeWidget>
#include <QWidget>

namespace
{

    struct RefRegistry
    {
        QMutex mutex;
        int nextId = 1;
        QHash<quintptr, QString> refByPtr;
        QHash<QString, QPointer<QWidget>> widgetByRef;
    };

    RefRegistry &registry()
    {
        static RefRegistry instance;
        return instance;
    }

    QString ensureRef(const QWidget *widget)
    {
        if (widget == nullptr)
        {
            return QString();
        }

        RefRegistry &r = registry();
        QMutexLocker locker(&r.mutex);
        const quintptr key = reinterpret_cast<quintptr>(widget);
        const auto existing = r.refByPtr.constFind(key);
        if (existing != r.refByPtr.constEnd())
        {
            return existing.value();
        }

        const QString ref = QStringLiteral("w%1").arg(r.nextId++);
        r.refByPtr.insert(key, ref);
        r.widgetByRef.insert(ref, const_cast<QWidget *>(widget));
        return ref;
    }

    void refreshRegistry()
    {
        const auto widgets = QApplication::allWidgets();
        RefRegistry &r = registry();
        QMutexLocker locker(&r.mutex);
        for (QWidget *widget : widgets)
        {
            if (widget == nullptr)
            {
                continue;
            }
            const quintptr key = reinterpret_cast<quintptr>(widget);
            if (!r.refByPtr.contains(key))
            {
                const QString ref = QStringLiteral("w%1").arg(r.nextId++);
                r.refByPtr.insert(key, ref);
                r.widgetByRef.insert(ref, const_cast<QWidget *>(widget));
            }
        }
    }

    QWidget *widgetByRefInternal(const QString &ref)
    {
        refreshRegistry();
        RefRegistry &r = registry();
        QMutexLocker locker(&r.mutex);
        const auto it = r.widgetByRef.constFind(ref);
        if (it == r.widgetByRef.constEnd())
        {
            return nullptr;
        }
        return it.value();
    }

    QJsonObject geometryToJson(const QRect &rect)
    {
        return QJsonObject{
            {"x", rect.x()},
            {"y", rect.y()},
            {"width", rect.width()},
            {"height", rect.height()},
        };
    }

    QJsonObject marginsToJson(const QMargins &margins)
    {
        return QJsonObject{
            {"left", margins.left()},
            {"top", margins.top()},
            {"right", margins.right()},
            {"bottom", margins.bottom()},
        };
    }

    QString alignmentToString(Qt::Alignment alignment)
    {
        QStringList parts;

        if (alignment & Qt::AlignLeft)
        {
            parts << QStringLiteral("left");
        }
        if (alignment & Qt::AlignRight)
        {
            parts << QStringLiteral("right");
        }
        if (alignment & Qt::AlignHCenter)
        {
            parts << QStringLiteral("hcenter");
        }
        if (alignment & Qt::AlignTop)
        {
            parts << QStringLiteral("top");
        }
        if (alignment & Qt::AlignBottom)
        {
            parts << QStringLiteral("bottom");
        }
        if (alignment & Qt::AlignVCenter)
        {
            parts << QStringLiteral("vcenter");
        }
        if (alignment & Qt::AlignCenter)
        {
            parts << QStringLiteral("center");
        }

        return parts.join(QStringLiteral("|"));
    }

    QString widgetText(const QWidget *widget)
    {
        if (const auto *label = qobject_cast<const QLabel *>(widget))
        {
            return label->text();
        }
        if (const auto *button = qobject_cast<const QAbstractButton *>(widget))
        {
            return button->text();
        }
        if (const auto *lineEdit = qobject_cast<const QLineEdit *>(widget))
        {
            return lineEdit->text();
        }
        if (const auto *plainTextEdit = qobject_cast<const QPlainTextEdit *>(widget))
        {
            return plainTextEdit->toPlainText();
        }
        if (const auto *textEdit = qobject_cast<const QTextEdit *>(widget))
        {
            return textEdit->toPlainText();
        }
        if (const auto *comboBox = qobject_cast<const QComboBox *>(widget))
        {
            return comboBox->currentText();
        }
        if (const auto *tabWidget = qobject_cast<const QTabWidget *>(widget))
        {
            return tabWidget->tabText(tabWidget->currentIndex());
        }
        if (const auto *tabBar = qobject_cast<const QTabBar *>(widget))
        {
            return tabBar->tabText(tabBar->currentIndex());
        }

        return widget->windowTitle();
    }

    QString placeholderText(const QWidget *widget)
    {
        if (const auto *lineEdit = qobject_cast<const QLineEdit *>(widget))
        {
            return lineEdit->placeholderText();
        }

        return QString();
    }

    QString colorRoleName(QPalette::ColorRole role)
    {
        switch (role)
        {
        case QPalette::WindowText:
            return QStringLiteral("WindowText");
        case QPalette::Button:
            return QStringLiteral("Button");
        case QPalette::Light:
            return QStringLiteral("Light");
        case QPalette::Midlight:
            return QStringLiteral("Midlight");
        case QPalette::Dark:
            return QStringLiteral("Dark");
        case QPalette::Mid:
            return QStringLiteral("Mid");
        case QPalette::Text:
            return QStringLiteral("Text");
        case QPalette::BrightText:
            return QStringLiteral("BrightText");
        case QPalette::ButtonText:
            return QStringLiteral("ButtonText");
        case QPalette::Base:
            return QStringLiteral("Base");
        case QPalette::Window:
            return QStringLiteral("Window");
        case QPalette::Shadow:
            return QStringLiteral("Shadow");
        case QPalette::Highlight:
            return QStringLiteral("Highlight");
        case QPalette::HighlightedText:
            return QStringLiteral("HighlightedText");
        case QPalette::Link:
            return QStringLiteral("Link");
        case QPalette::LinkVisited:
            return QStringLiteral("LinkVisited");
        case QPalette::AlternateBase:
            return QStringLiteral("AlternateBase");
        case QPalette::ToolTipBase:
            return QStringLiteral("ToolTipBase");
        case QPalette::ToolTipText:
            return QStringLiteral("ToolTipText");
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        case QPalette::PlaceholderText:
            return QStringLiteral("PlaceholderText");
#endif
        case QPalette::NoRole:
        default:
            return QStringLiteral("NoRole");
        }
    }

    QString colorGroupName(QPalette::ColorGroup group)
    {
        switch (group)
        {
        case QPalette::Active:
            return QStringLiteral("active");
        case QPalette::Inactive:
            return QStringLiteral("inactive");
        case QPalette::Disabled:
            return QStringLiteral("disabled");
        case QPalette::Current:
        default:
            return QStringLiteral("current");
        }
    }

    QJsonObject colorToJson(const QColor &color)
    {
        const QString hex = color.isValid()
                                ? QStringLiteral("#%1%2%3%4")
                                      .arg(color.red(), 2, 16, QLatin1Char('0'))
                                      .arg(color.green(), 2, 16, QLatin1Char('0'))
                                      .arg(color.blue(), 2, 16, QLatin1Char('0'))
                                      .arg(color.alpha(), 2, 16, QLatin1Char('0'))
                                      .toUpper()
                                : QString();

        return QJsonObject{
            {"hex", hex},
            {"rgba", QJsonObject{
                         {"r", color.red()},
                         {"g", color.green()},
                         {"b", color.blue()},
                         {"a", color.alpha()},
                     }},
            {"valid", color.isValid()},
        };
    }

    QPalette::ColorGroup currentColorGroupForWidget(const QWidget *widget)
    {
        if (widget == nullptr)
        {
            return QPalette::Active;
        }
        if (!widget->isEnabled())
        {
            return QPalette::Disabled;
        }
        return widget->window() == QApplication::activeWindow() ? QPalette::Active : QPalette::Inactive;
    }

    QJsonObject paletteGroupToJson(const QPalette &palette, QPalette::ColorGroup group)
    {
        QJsonObject object{
            {"window", colorToJson(palette.color(group, QPalette::Window))},
            {"windowText", colorToJson(palette.color(group, QPalette::WindowText))},
            {"base", colorToJson(palette.color(group, QPalette::Base))},
            {"alternateBase", colorToJson(palette.color(group, QPalette::AlternateBase))},
            {"text", colorToJson(palette.color(group, QPalette::Text))},
            {"button", colorToJson(palette.color(group, QPalette::Button))},
            {"buttonText", colorToJson(palette.color(group, QPalette::ButtonText))},
            {"highlight", colorToJson(palette.color(group, QPalette::Highlight))},
            {"highlightedText", colorToJson(palette.color(group, QPalette::HighlightedText))},
            {"toolTipBase", colorToJson(palette.color(group, QPalette::ToolTipBase))},
            {"toolTipText", colorToJson(palette.color(group, QPalette::ToolTipText))},
        };

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        object.insert(QStringLiteral("placeholderText"), colorToJson(palette.color(group, QPalette::PlaceholderText)));
#else
        object.insert(QStringLiteral("placeholderText"), colorToJson(QColor()));
#endif

        return object;
    }

    bool inheritsStyleSheetImpl(const QWidget *widget)
    {
        if (widget == nullptr || !widget->styleSheet().isEmpty())
        {
            return false;
        }

        const QWidget *current = widget->parentWidget();
        while (current != nullptr)
        {
            if (!current->styleSheet().isEmpty())
            {
                return true;
            }
            current = current->parentWidget();
        }

        return false;
    }

    QJsonObject roleSummary(const QWidget *widget, const QPalette &palette, QPalette::ColorGroup group)
    {
        const QPalette::ColorRole foregroundRole = widget != nullptr ? widget->foregroundRole() : QPalette::NoRole;
        const QPalette::ColorRole backgroundRole = widget != nullptr ? widget->backgroundRole() : QPalette::NoRole;

        return QJsonObject{
            {"foregroundRole", colorRoleName(foregroundRole)},
            {"foregroundColor", colorToJson(foregroundRole == QPalette::NoRole ? QColor() : palette.color(group, foregroundRole))},
            {"backgroundRole", colorRoleName(backgroundRole)},
            {"backgroundColor", colorToJson(backgroundRole == QPalette::NoRole ? QColor() : palette.color(group, backgroundRole))},
        };
    }

    QJsonObject styleAttachmentNode(const QWidget *widget);

    QJsonObject classHintsForWidget(const QWidget *widget, const QPalette &palette, QPalette::ColorGroup group)
    {
        QJsonObject hints;
        if (widget == nullptr)
        {
            return hints;
        }

        if (qobject_cast<const QLabel *>(widget) != nullptr)
        {
            hints.insert(QStringLiteral("textColor"), colorToJson(palette.color(group, QPalette::WindowText)));
        }
        else if (qobject_cast<const QPushButton *>(widget) != nullptr)
        {
            hints.insert(QStringLiteral("buttonFill"), colorToJson(palette.color(group, QPalette::Button)));
            hints.insert(QStringLiteral("textColor"), colorToJson(palette.color(group, QPalette::ButtonText)));
        }
        else if (qobject_cast<const QLineEdit *>(widget) != nullptr)
        {
            hints.insert(QStringLiteral("textColor"), colorToJson(palette.color(group, QPalette::Text)));
            hints.insert(QStringLiteral("baseFill"), colorToJson(palette.color(group, QPalette::Base)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
            hints.insert(QStringLiteral("placeholderTextColor"),
                         colorToJson(palette.color(group, QPalette::PlaceholderText)));
#else
            hints.insert(QStringLiteral("placeholderTextColor"), colorToJson(QColor()));
#endif
        }
        else if (qobject_cast<const QCheckBox *>(widget) != nullptr ||
                 qobject_cast<const QRadioButton *>(widget) != nullptr)
        {
            hints.insert(QStringLiteral("textColor"), colorToJson(palette.color(group, QPalette::WindowText)));
        }
        else if (qobject_cast<const QComboBox *>(widget) != nullptr)
        {
            hints.insert(QStringLiteral("textColor"), colorToJson(palette.color(group, QPalette::Text)));
            hints.insert(QStringLiteral("baseFill"), colorToJson(palette.color(group, QPalette::Base)));
            hints.insert(QStringLiteral("buttonFill"), colorToJson(palette.color(group, QPalette::Button)));
        }
        else if (qobject_cast<const QGroupBox *>(widget) != nullptr)
        {
            hints.insert(QStringLiteral("titleTextColor"), colorToJson(palette.color(group, QPalette::WindowText)));
        }
        else if (qobject_cast<const QProgressBar *>(widget) != nullptr)
        {
            hints.insert(QStringLiteral("textColor"), colorToJson(palette.color(group, QPalette::Text)));
            hints.insert(QStringLiteral("baseFill"), colorToJson(palette.color(group, QPalette::Base)));
            hints.insert(QStringLiteral("highlightFill"), colorToJson(palette.color(group, QPalette::Highlight)));
        }
        else if (qobject_cast<const QListWidget *>(widget) != nullptr ||
                 qobject_cast<const QTreeWidget *>(widget) != nullptr ||
                 qobject_cast<const QTableWidget *>(widget) != nullptr)
        {
            hints.insert(QStringLiteral("baseFill"), colorToJson(palette.color(group, QPalette::Base)));
            hints.insert(QStringLiteral("textColor"), colorToJson(palette.color(group, QPalette::Text)));
            hints.insert(QStringLiteral("alternateBaseFill"), colorToJson(palette.color(group, QPalette::AlternateBase)));
            hints.insert(QStringLiteral("highlightFill"), colorToJson(palette.color(group, QPalette::Highlight)));
            hints.insert(QStringLiteral("highlightedTextColor"),
                         colorToJson(palette.color(group, QPalette::HighlightedText)));
        }

        return hints;
    }

    QList<QWidget *> directWidgetChildren(const QWidget *widget)
    {
        QList<QWidget *> children;
        const auto childObjects = widget->children();

        for (QObject *childObject : childObjects)
        {
            auto *childWidget = qobject_cast<QWidget *>(childObject);
            if (childWidget == nullptr || childWidget->parentWidget() != widget)
            {
                continue;
            }
            children.append(childWidget);
        }

        return children;
    }

    bool isDescendantOrSelf(const QWidget *widget, const QWidget *ancestor)
    {
        const QWidget *current = widget;
        while (current != nullptr)
        {
            if (current == ancestor)
            {
                return true;
            }
            current = current->parentWidget();
        }

        return false;
    }

    int indexInParentImpl(const QWidget *widget)
    {
        const QWidget *parent = widget != nullptr ? widget->parentWidget() : nullptr;
        if (parent == nullptr)
        {
            return -1;
        }

        const QList<QWidget *> siblings = directWidgetChildren(parent);
        for (int i = 0; i < siblings.size(); ++i)
        {
            if (siblings.at(i) == widget)
            {
                return i;
            }
        }

        return -1;
    }

    QString pageContainerType(const QWidget *widget)
    {
        if (qobject_cast<const QTabWidget *>(widget) != nullptr)
        {
            return QStringLiteral("tab");
        }
        if (qobject_cast<const QStackedWidget *>(widget) != nullptr)
        {
            return QStringLiteral("stacked");
        }
        if (qobject_cast<const QToolBox *>(widget) != nullptr)
        {
            return QStringLiteral("toolbox");
        }
        if (qobject_cast<const QScrollArea *>(widget) != nullptr ||
            qobject_cast<const QAbstractScrollArea *>(widget) != nullptr)
        {
            return QStringLiteral("scroll");
        }
        if (qobject_cast<const QDialog *>(widget) != nullptr)
        {
            return QStringLiteral("dialog");
        }
        if (qobject_cast<const QMainWindow *>(widget) != nullptr)
        {
            return QStringLiteral("main_window");
        }

        return QString();
    }

    bool isInputWidgetImpl(const QWidget *widget)
    {
        if (const auto *lineEdit = qobject_cast<const QLineEdit *>(widget))
        {
            return !lineEdit->isReadOnly();
        }
        if (const auto *plainTextEdit = qobject_cast<const QPlainTextEdit *>(widget))
        {
            return !plainTextEdit->isReadOnly();
        }
        if (const auto *textEdit = qobject_cast<const QTextEdit *>(widget))
        {
            return !textEdit->isReadOnly();
        }
        if (qobject_cast<const QComboBox *>(widget) != nullptr)
        {
            return true;
        }

        return false;
    }

    bool isContainerWidgetImpl(const QWidget *widget)
    {
        if (widget == nullptr)
        {
            return false;
        }

        if (!directWidgetChildren(widget).isEmpty())
        {
            return true;
        }

        if (qobject_cast<const QTabWidget *>(widget) != nullptr ||
            qobject_cast<const QStackedWidget *>(widget) != nullptr ||
            qobject_cast<const QToolBox *>(widget) != nullptr ||
            qobject_cast<const QScrollArea *>(widget) != nullptr ||
            widget->layout() != nullptr)
        {
            return true;
        }

        return false;
    }

    QJsonArray availableActions(const QWidget *widget)
    {
        QJsonArray actions;

        if (qobject_cast<const QPushButton *>(widget) != nullptr ||
            (qobject_cast<const QLabel *>(widget) == nullptr && qobject_cast<const QAbstractButton *>(widget) != nullptr))
        {
            actions.append(QStringLiteral("click"));
        }

        if (const auto *button = qobject_cast<const QAbstractButton *>(widget))
        {
            if (button->isCheckable())
            {
                actions.append(QStringLiteral("toggle_check"));
            }
        }

        if (const auto *lineEdit = qobject_cast<const QLineEdit *>(widget))
        {
            if (!lineEdit->isReadOnly())
            {
                actions.append(QStringLiteral("set_text"));
                actions.append(QStringLiteral("press_key"));
                actions.append(QStringLiteral("send_shortcut"));
            }
        }
        else if (const auto *plainTextEdit = qobject_cast<const QPlainTextEdit *>(widget))
        {
            if (!plainTextEdit->isReadOnly())
            {
                actions.append(QStringLiteral("set_text"));
                actions.append(QStringLiteral("press_key"));
                actions.append(QStringLiteral("send_shortcut"));
            }
        }
        else if (const auto *textEdit = qobject_cast<const QTextEdit *>(widget))
        {
            if (!textEdit->isReadOnly())
            {
                actions.append(QStringLiteral("set_text"));
                actions.append(QStringLiteral("press_key"));
                actions.append(QStringLiteral("send_shortcut"));
            }
        }

        if (qobject_cast<const QComboBox *>(widget) != nullptr)
        {
            actions.append(QStringLiteral("choose_combo_option"));
        }

        if (qobject_cast<const QListWidget *>(widget) != nullptr ||
            qobject_cast<const QTreeWidget *>(widget) != nullptr ||
            qobject_cast<const QTableWidget *>(widget) != nullptr)
        {
            actions.append(QStringLiteral("select_item"));
            actions.append(QStringLiteral("scroll"));
        }

        if (qobject_cast<const QTabWidget *>(widget) != nullptr ||
            qobject_cast<const QTabBar *>(widget) != nullptr)
        {
            actions.append(QStringLiteral("activate_tab"));
        }

        if (qobject_cast<const QStackedWidget *>(widget) != nullptr)
        {
            actions.append(QStringLiteral("switch_stacked_page"));
        }

        if (qobject_cast<const QTreeWidget *>(widget) != nullptr)
        {
            actions.append(QStringLiteral("expand_tree_node"));
            actions.append(QStringLiteral("collapse_tree_node"));
        }

        if (qobject_cast<const QAbstractScrollArea *>(widget) != nullptr ||
            qobject_cast<const QScrollArea *>(widget) != nullptr)
        {
            actions.append(QStringLiteral("scroll"));
        }

        if (widget != nullptr && widget->focusPolicy() != Qt::NoFocus)
        {
            actions.append(QStringLiteral("press_key"));
            actions.append(QStringLiteral("send_shortcut"));
        }

        return actions;
    }

    QString riskLevel(const QWidget *widget, const QJsonArray &actions)
    {
        if (widget == nullptr || !widget->isVisible())
        {
            return QStringLiteral("unknown");
        }
        if (!actions.isEmpty())
        {
            return QStringLiteral("safe_action");
        }
        return QStringLiteral("safe_read");
    }

    QString pathSegment(const QWidget *widget)
    {
        if (!widget->objectName().isEmpty())
        {
            return widget->objectName();
        }

        return QStringLiteral("%1#%2")
            .arg(QString::fromUtf8(widget->metaObject()->className()))
            .arg(indexInParentImpl(widget));
    }

    QString widgetPath(const QWidget *widget)
    {
        QStringList segments;
        const QWidget *current = widget;

        while (current != nullptr)
        {
            segments.prepend(pathSegment(current));
            current = current->parentWidget();
        }

        return QStringLiteral("/") + segments.join(QStringLiteral("/"));
    }

    int widgetDepth(const QWidget *widget)
    {
        int depth = 0;
        const QWidget *current = widget != nullptr ? widget->parentWidget() : nullptr;
        while (current != nullptr)
        {
            ++depth;
            current = current->parentWidget();
        }

        return depth;
    }

    QWidget *scrollContainerForWidgetImpl(const QWidget *widget)
    {
        QWidget *current = widget != nullptr ? widget->parentWidget() : nullptr;
        while (current != nullptr)
        {
            if (qobject_cast<QScrollArea *>(current) != nullptr ||
                qobject_cast<QAbstractScrollArea *>(current) != nullptr)
            {
                return current;
            }
            current = current->parentWidget();
        }

        return nullptr;
    }

    bool isOnCurrentPageImpl(const QWidget *widget)
    {
        if (widget == nullptr)
        {
            return false;
        }

        const QWidget *current = widget;
        while (current != nullptr)
        {
            const QWidget *parent = current->parentWidget();
            if (const auto *tabWidget = qobject_cast<const QTabWidget *>(parent))
            {
                const QWidget *page = tabWidget->currentWidget();
                if (page != nullptr && !isDescendantOrSelf(widget, page))
                {
                    return false;
                }
            }
            if (const auto *stackedWidget = qobject_cast<const QStackedWidget *>(parent))
            {
                const QWidget *page = stackedWidget->currentWidget();
                if (page != nullptr && !isDescendantOrSelf(widget, page))
                {
                    return false;
                }
            }
            if (const auto *toolBox = qobject_cast<const QToolBox *>(parent))
            {
                const QWidget *page = toolBox->currentWidget();
                if (page != nullptr && !isDescendantOrSelf(widget, page))
                {
                    return false;
                }
            }
            current = parent;
        }

        return true;
    }

    QWidget *activePageWidgetImpl()
    {
        auto resolveFromWidget = [](QWidget *widget) -> QWidget *
        {
            QWidget *current = widget;
            while (current != nullptr)
            {
                if (auto *tabWidget = qobject_cast<QTabWidget *>(current))
                {
                    return tabWidget->currentWidget();
                }
                if (auto *stackedWidget = qobject_cast<QStackedWidget *>(current))
                {
                    return stackedWidget->currentWidget();
                }
                if (auto *toolBox = qobject_cast<QToolBox *>(current))
                {
                    return toolBox->currentWidget();
                }
                current = current->parentWidget();
            }
            return nullptr;
        };

        if (QWidget *focusWidget = QApplication::focusWidget())
        {
            if (QWidget *page = resolveFromWidget(focusWidget))
            {
                return page;
            }
        }

        if (QWidget *activeWindow = QApplication::activeWindow())
        {
            if (QWidget *page = resolveFromWidget(activeWindow))
            {
                return page;
            }

            const auto widgets = QApplication::allWidgets();
            for (QWidget *widget : widgets)
            {
                if (widget != nullptr && widget->window() == activeWindow)
                {
                    if (QWidget *page = resolveFromWidget(widget))
                    {
                        return page;
                    }
                }
            }
        }

        return nullptr;
    }

    QList<QWidget *> structuralChildren(const QWidget *widget)
    {
        QList<QWidget *> children;
        if (widget == nullptr)
        {
            return children;
        }

        if (const auto *mainWindow = qobject_cast<const QMainWindow *>(widget))
        {
            if (QWidget *central = mainWindow->centralWidget())
            {
                children.append(central);
            }
            const QList<QWidget *> direct = directWidgetChildren(widget);
            for (QWidget *child : direct)
            {
                if (!children.contains(child) &&
                    (qobject_cast<QDialog *>(child) != nullptr || child->inherits("QDockWidget")))
                {
                    children.append(child);
                }
            }
            return children;
        }

        if (const auto *tabWidget = qobject_cast<const QTabWidget *>(widget))
        {
            for (int i = 0; i < tabWidget->count(); ++i)
            {
                if (QWidget *page = tabWidget->widget(i))
                {
                    children.append(page);
                }
            }
            return children;
        }

        if (const auto *stackedWidget = qobject_cast<const QStackedWidget *>(widget))
        {
            for (int i = 0; i < stackedWidget->count(); ++i)
            {
                if (QWidget *page = stackedWidget->widget(i))
                {
                    children.append(page);
                }
            }
            return children;
        }

        if (const auto *toolBox = qobject_cast<const QToolBox *>(widget))
        {
            for (int i = 0; i < toolBox->count(); ++i)
            {
                if (QWidget *page = toolBox->widget(i))
                {
                    children.append(page);
                }
            }
            return children;
        }

        if (const auto *scrollArea = qobject_cast<const QScrollArea *>(widget))
        {
            if (QWidget *content = scrollArea->widget())
            {
                children.append(content);
            }
            return children;
        }

        return directWidgetChildren(widget);
    }

    QString layoutTypeForWidget(const QWidget *widget)
    {
        if (widget == nullptr || widget->layout() == nullptr)
        {
            return QString();
        }

        return QString::fromUtf8(widget->layout()->metaObject()->className());
    }

    QJsonObject parentLayoutContext(const QWidget *widget)
    {
        QJsonObject context{
            {"indexInParent", indexInParentImpl(widget)},
            {"row", -1},
            {"column", -1},
            {"rowSpan", 1},
            {"columnSpan", 1},
            {"stretch", -1},
            {"alignment", QString()},
        };

        const QWidget *parent = widget != nullptr ? widget->parentWidget() : nullptr;
        if (parent == nullptr || parent->layout() == nullptr)
        {
            return context;
        }

        QLayout *layout = parent->layout();
        context.insert(QStringLiteral("parentLayoutType"), QString::fromUtf8(layout->metaObject()->className()));

        const int index = layout->indexOf(const_cast<QWidget *>(widget));
        if (index < 0)
        {
            return context;
        }

        if (auto *grid = qobject_cast<QGridLayout *>(layout))
        {
            int row = -1;
            int column = -1;
            int rowSpan = 1;
            int columnSpan = 1;
            grid->getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
            context.insert(QStringLiteral("row"), row);
            context.insert(QStringLiteral("column"), column);
            context.insert(QStringLiteral("rowSpan"), rowSpan);
            context.insert(QStringLiteral("columnSpan"), columnSpan);
        }

        if (auto *box = qobject_cast<QBoxLayout *>(layout))
        {
            context.insert(QStringLiteral("stretch"), box->stretch(index));
        }

        if (QLayoutItem *item = layout->itemAt(index))
        {
            context.insert(QStringLiteral("alignment"), alignmentToString(item->alignment()));
        }

        return context;
    }

    QString layoutRoleForWidget(const QWidget *widget)
    {
        if (widget == nullptr)
        {
            return QStringLiteral("leaf");
        }
        if (widget->isWindow())
        {
            return QStringLiteral("window");
        }
        if (qobject_cast<const QScrollArea *>(widget) != nullptr ||
            qobject_cast<const QAbstractScrollArea *>(widget) != nullptr)
        {
            return QStringLiteral("scroll_container");
        }
        if (!pageContainerType(widget).isEmpty())
        {
            if (qobject_cast<const QDialog *>(widget) != nullptr)
            {
                return QStringLiteral("dialog");
            }
            return QStringLiteral("page_container");
        }
        if (widget == activePageWidgetImpl())
        {
            return QStringLiteral("page");
        }
        if (widget->layout() != nullptr || isContainerWidgetImpl(widget))
        {
            return QStringLiteral("container");
        }
        return QStringLiteral("leaf");
    }

    bool isCurrentPageWidget(const QWidget *widget)
    {
        QWidget *activePage = activePageWidgetImpl();
        if (activePage == nullptr || widget == nullptr)
        {
            return false;
        }

        return widget == activePage || isDescendantOrSelf(activePage, widget);
    }

    QJsonObject widgetSummaryImpl(const QWidget *widget)
    {
        const QJsonArray actions = availableActions(widget);
        const QWidget *activeWindow = QApplication::activeWindow();

        return QJsonObject{
            {"ref", ensureRef(widget)},
            {"objectName", widget->objectName()},
            {"className", QString::fromUtf8(widget->metaObject()->className())},
            {"text", widgetText(widget)},
            {"placeholderText", placeholderText(widget)},
            {"windowTitle", widget->windowTitle()},
            {"visible", widget->isVisible()},
            {"enabled", widget->isEnabled()},
            {"path", widgetPath(widget)},
            {"depth", widgetDepth(widget)},
            {"indexInParent", indexInParentImpl(widget)},
            {"isInteractive", !actions.isEmpty()},
            {"isInput", isInputWidgetImpl(widget)},
            {"isContainer", isContainerWidgetImpl(widget)},
            {"isCurrentPage", isCurrentPageWidget(widget)},
            {"hasFocus", QApplication::focusWidget() == widget},
            {"isActiveWindow", widget->window() == activeWindow},
            {"pageContainerType", pageContainerType(widget)},
            {"riskLevel", riskLevel(widget, actions)},
            {"geometry", geometryToJson(widget->geometry())},
            {"availableActions", actions},
        };
    }

    QJsonObject layoutSummaryImpl(const QWidget *widget)
    {
        const QJsonObject parentContext = parentLayoutContext(widget);
        const QMargins margins = widget->layout() != nullptr ? widget->layout()->contentsMargins() : QMargins();
        const int spacing = widget->layout() != nullptr ? widget->layout()->spacing() : -1;

        return QJsonObject{
            {"ref", ensureRef(widget)},
            {"ownerRef", ensureRef(widget)},
            {"objectName", widget->objectName()},
            {"className", QString::fromUtf8(widget->metaObject()->className())},
            {"layoutType", layoutTypeForWidget(widget)},
            {"layoutRole", layoutRoleForWidget(widget)},
            {"indexInParent", parentContext.value(QStringLiteral("indexInParent"))},
            {"row", parentContext.value(QStringLiteral("row"))},
            {"column", parentContext.value(QStringLiteral("column"))},
            {"rowSpan", parentContext.value(QStringLiteral("rowSpan"))},
            {"columnSpan", parentContext.value(QStringLiteral("columnSpan"))},
            {"stretch", parentContext.value(QStringLiteral("stretch"))},
            {"alignment", parentContext.value(QStringLiteral("alignment"))},
            {"margins", marginsToJson(margins)},
            {"spacing", spacing},
            {"isScrollArea", qobject_cast<const QScrollArea *>(widget) != nullptr ||
                                 qobject_cast<const QAbstractScrollArea *>(widget) != nullptr},
            {"isCurrentPage", isCurrentPageWidget(widget)},
            {"pageContainerType", pageContainerType(widget)},
        };
    }

    QJsonObject styleSummaryImpl(const QWidget *widget)
    {
        if (widget == nullptr)
        {
            return QJsonObject{};
        }

        const QPalette palette = widget->palette();
        const QPalette::ColorGroup currentGroup = currentColorGroupForWidget(widget);

        QJsonObject style{
            {"styleSheet", widget->styleSheet()},
            {"inheritsStyleSheet", inheritsStyleSheetImpl(widget)},
            {"autoFillBackground", widget->autoFillBackground()},
            {"currentColorGroup", colorGroupName(currentGroup)},
            {"roles", roleSummary(widget, palette, currentGroup)},
            {"palette", QJsonObject{
                            {"current", paletteGroupToJson(palette, currentGroup)},
                            {"active", paletteGroupToJson(palette, QPalette::Active)},
                            {"inactive", paletteGroupToJson(palette, QPalette::Inactive)},
                            {"disabled", paletteGroupToJson(palette, QPalette::Disabled)},
                        }},
            {"classHints", classHintsForWidget(widget, palette, currentGroup)},
        };

        if (const auto *scrollArea = qobject_cast<const QAbstractScrollArea *>(widget))
        {
            if (QWidget *viewport = scrollArea->viewport())
            {
                style.insert(QStringLiteral("viewport"), styleAttachmentNode(viewport));
            }
        }

        if (const auto *treeWidget = qobject_cast<const QTreeWidget *>(widget))
        {
            style.insert(QStringLiteral("header"), styleAttachmentNode(treeWidget->header()));
        }
        else if (const auto *tableWidget = qobject_cast<const QTableWidget *>(widget))
        {
            style.insert(QStringLiteral("horizontalHeader"), styleAttachmentNode(tableWidget->horizontalHeader()));
            style.insert(QStringLiteral("verticalHeader"), styleAttachmentNode(tableWidget->verticalHeader()));
        }

        return style;
    }

    QJsonObject styleAttachmentNode(const QWidget *widget)
    {
        if (widget == nullptr)
        {
            return QJsonObject{};
        }

        return QJsonObject{
            {"widget", widgetSummaryImpl(widget)},
            {"style", styleSummaryImpl(widget)},
        };
    }

    QJsonObject objectTreeNode(const QWidget *widget, bool visibleOnly)
    {
        QJsonObject node = widgetSummaryImpl(widget);
        QJsonArray children;

        const auto childWidgets = structuralChildren(widget);
        for (QWidget *childWidget : childWidgets)
        {
            if (childWidget == nullptr)
            {
                continue;
            }
            if (visibleOnly && !childWidget->isVisible())
            {
                continue;
            }

            children.append(objectTreeNode(childWidget, visibleOnly));
        }

        node.insert(QStringLiteral("children"), children);
        return node;
    }

    QJsonObject styleTreeNode(const QWidget *widget, bool includeChildren)
    {
        QJsonObject node{
            {"widget", widgetSummaryImpl(widget)},
            {"style", styleSummaryImpl(widget)},
        };

        if (includeChildren)
        {
            QJsonArray children;
            const auto childWidgets = structuralChildren(widget);
            for (QWidget *childWidget : childWidgets)
            {
                if (childWidget == nullptr)
                {
                    continue;
                }
                children.append(styleTreeNode(childWidget, true));
            }
            node.insert(QStringLiteral("children"), children);
        }

        return node;
    }

    QJsonObject layoutTreeNode(const QWidget *widget, bool visibleOnly)
    {
        QJsonObject node = layoutSummaryImpl(widget);
        QJsonArray children;

        const auto childWidgets = structuralChildren(widget);
        for (QWidget *childWidget : childWidgets)
        {
            if (childWidget == nullptr)
            {
                continue;
            }
            if (visibleOnly && !childWidget->isVisible())
            {
                continue;
            }

            children.append(layoutTreeNode(childWidget, visibleOnly));
        }

        node.insert(QStringLiteral("children"), children);
        return node;
    }

    QList<QWidget *> candidateWidgets()
    {
        refreshRegistry();
        return QApplication::allWidgets();
    }

    bool matchesAncestorRef(const QWidget *widget, const QString &ref)
    {
        const QWidget *current = widget != nullptr ? widget->parentWidget() : nullptr;
        while (current != nullptr)
        {
            if (ensureRef(current) == ref)
            {
                return true;
            }
            current = current->parentWidget();
        }

        return false;
    }

    bool matchesAncestorObjectName(const QWidget *widget, const QString &objectName)
    {
        const QWidget *current = widget != nullptr ? widget->parentWidget() : nullptr;
        while (current != nullptr)
        {
            if (current->objectName() == objectName)
            {
                return true;
            }
            current = current->parentWidget();
        }

        return false;
    }

    bool matchesSelectorImpl(const QWidget *widget, const QJsonObject &selector)
    {
        if (selector.contains(QStringLiteral("ref")) &&
            ensureRef(widget) != selector.value(QStringLiteral("ref")).toString())
        {
            return false;
        }

        if (selector.contains(QStringLiteral("path")) &&
            widgetPath(widget) != selector.value(QStringLiteral("path")).toString())
        {
            return false;
        }

        if (selector.contains(QStringLiteral("objectName")) &&
            widget->objectName() != selector.value(QStringLiteral("objectName")).toString())
        {
            return false;
        }

        if (selector.contains(QStringLiteral("className")) &&
            QString::fromUtf8(widget->metaObject()->className()) != selector.value(QStringLiteral("className")).toString())
        {
            return false;
        }

        const QString text = widgetText(widget);
        if (selector.contains(QStringLiteral("textEquals")) &&
            text != selector.value(QStringLiteral("textEquals")).toString())
        {
            return false;
        }

        if (selector.contains(QStringLiteral("textContains")) &&
            !text.contains(selector.value(QStringLiteral("textContains")).toString(), Qt::CaseInsensitive))
        {
            return false;
        }

        if (selector.contains(QStringLiteral("placeholderText")) &&
            placeholderText(widget) != selector.value(QStringLiteral("placeholderText")).toString())
        {
            return false;
        }

        const QString windowTitle = widget->window()->windowTitle();
        if (selector.contains(QStringLiteral("windowTitleContains")) &&
            !windowTitle.contains(selector.value(QStringLiteral("windowTitleContains")).toString(), Qt::CaseInsensitive))
        {
            return false;
        }

        if (selector.contains(QStringLiteral("windowObjectName")) &&
            widget->window()->objectName() != selector.value(QStringLiteral("windowObjectName")).toString())
        {
            return false;
        }

        if (selector.contains(QStringLiteral("isActiveWindow")) &&
            (widget->window() == QApplication::activeWindow()) != selector.value(QStringLiteral("isActiveWindow")).toBool())
        {
            return false;
        }

        if (selector.contains(QStringLiteral("ancestorRef")) &&
            !matchesAncestorRef(widget, selector.value(QStringLiteral("ancestorRef")).toString()))
        {
            return false;
        }

        if (selector.contains(QStringLiteral("ancestorObjectName")) &&
            !matchesAncestorObjectName(widget, selector.value(QStringLiteral("ancestorObjectName")).toString()))
        {
            return false;
        }

        if (selector.contains(QStringLiteral("currentPageOnly")) &&
            selector.value(QStringLiteral("currentPageOnly")).toBool() &&
            !isOnCurrentPageImpl(widget))
        {
            return false;
        }

        if (selector.contains(QStringLiteral("visible")) &&
            widget->isVisible() != selector.value(QStringLiteral("visible")).toBool())
        {
            return false;
        }

        if (selector.contains(QStringLiteral("enabled")) &&
            widget->isEnabled() != selector.value(QStringLiteral("enabled")).toBool())
        {
            return false;
        }

        if (selector.contains(QStringLiteral("indexInParent")) &&
            indexInParentImpl(widget) != selector.value(QStringLiteral("indexInParent")).toInt())
        {
            return false;
        }

        return true;
    }

} // namespace

namespace WidgetIntrospection
{

    QJsonObject describeUi()
    {
        return describeSnapshot();
    }

    QJsonObject describeSnapshot()
    {
        refreshRegistry();

        QJsonArray visibleWindows;
        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget *widget : topLevels)
        {
            if (widget == nullptr || !widget->isVisible())
            {
                continue;
            }
            visibleWindows.append(objectTreeNode(widget, true));
        }

        QJsonObject result{
            {"windows", visibleWindows},
            {"topLevelWindows", listWindows()},
            {"widgetCount", QApplication::allWidgets().size()},
        };

        if (QWidget *focusWidget = QApplication::focusWidget())
        {
            result.insert(QStringLiteral("focusWidget"), widgetSummary(focusWidget));
        }

        if (QWidget *activeWindow = QApplication::activeWindow())
        {
            result.insert(QStringLiteral("activeWindow"), widgetSummary(activeWindow));
            result.insert(QStringLiteral("visibleSubtree"), objectTreeNode(activeWindow, true));
        }

        if (QWidget *modalDialog = QApplication::activeModalWidget())
        {
            result.insert(QStringLiteral("modalDialog"), widgetSummary(modalDialog));
        }

        if (QWidget *page = activePageWidgetImpl())
        {
            result.insert(QStringLiteral("activePage"), widgetSummary(page));
        }

        return result;
    }

    QJsonObject describeObjectTree(bool visibleOnly)
    {
        refreshRegistry();

        QJsonArray windows;
        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget *widget : topLevels)
        {
            if (widget == nullptr || !widget->isVisible())
            {
                continue;
            }
            windows.append(objectTreeNode(widget, visibleOnly));
        }

        return QJsonObject{
            {"treeType", QStringLiteral("object")},
            {"visibleOnly", visibleOnly},
            {"windows", windows},
        };
    }

    QJsonObject describeLayoutTree(bool visibleOnly)
    {
        refreshRegistry();

        QJsonArray windows;
        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget *widget : topLevels)
        {
            if (widget == nullptr || !widget->isVisible())
            {
                continue;
            }
            windows.append(layoutTreeNode(widget, visibleOnly));
        }

        return QJsonObject{
            {"treeType", QStringLiteral("layout")},
            {"visibleOnly", visibleOnly},
            {"windows", windows},
        };
    }

    QJsonObject describeSubtree(const QJsonObject &selector, bool layoutTree, bool visibleOnly,
                                QString *errorCode, QString *errorMessage, QJsonArray *candidates)
    {
        QString localErrorMessage;
        QJsonArray localCandidates;
        QWidget *widget = findSingleWidget(selector, &localErrorMessage, &localCandidates);
        if (widget == nullptr)
        {
            if (errorCode != nullptr)
            {
                *errorCode = localCandidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector");
            }
            if (errorMessage != nullptr)
            {
                *errorMessage = localErrorMessage;
            }
            if (candidates != nullptr)
            {
                *candidates = localCandidates;
            }
            return QJsonObject{};
        }

        return layoutTree ? layoutTreeNode(widget, visibleOnly) : objectTreeNode(widget, visibleOnly);
    }

    QJsonObject describeStyle(const QJsonObject &selector, bool includeChildren,
                              QString *errorCode, QString *errorMessage, QJsonArray *candidates)
    {
        QString localErrorMessage;
        QJsonArray localCandidates;
        QWidget *widget = findSingleWidget(selector, &localErrorMessage, &localCandidates);
        if (widget == nullptr)
        {
            if (errorCode != nullptr)
            {
                *errorCode = localCandidates.isEmpty() ? QStringLiteral("not_found") : QStringLiteral("ambiguous_selector");
            }
            if (errorMessage != nullptr)
            {
                *errorMessage = localErrorMessage;
            }
            if (candidates != nullptr)
            {
                *candidates = localCandidates;
            }
            return QJsonObject{};
        }

        return QJsonObject{
            {"treeType", QStringLiteral("style")},
            {"includeChildren", includeChildren},
            {"root", styleTreeNode(widget, includeChildren)},
        };
    }

    QJsonObject describeActivePage()
    {
        if (QWidget *page = activePageWidgetImpl())
        {
            return QJsonObject{
                {"page", widgetSummary(page)},
                {"objectTree", objectTreeNode(page, true)},
                {"layoutTree", layoutTreeNode(page, true)},
            };
        }

        return QJsonObject{};
    }

    QJsonArray listWindows()
    {
        refreshRegistry();

        QJsonArray windows;
        const auto topLevels = QApplication::topLevelWidgets();
        QWidget *activeWindow = QApplication::activeWindow();
        QWidget *modalDialog = QApplication::activeModalWidget();

        for (QWidget *widget : topLevels)
        {
            if (widget == nullptr || !widget->isVisible())
            {
                continue;
            }

            QJsonObject summary = widgetSummary(widget);
            summary.insert(QStringLiteral("active"), widget == activeWindow);
            summary.insert(QStringLiteral("modal"), widget == modalDialog);
            windows.append(summary);
        }

        return windows;
    }

    QJsonArray findWidgets(const QJsonObject &selector)
    {
        refreshRegistry();

        QJsonArray result;
        const QList<QWidget *> widgets = candidateWidgets();
        for (QWidget *widget : widgets)
        {
            if (widget != nullptr && matchesSelectorImpl(widget, selector))
            {
                result.append(widgetSummary(widget));
            }
        }

        return result;
    }

    QWidget *findSingleWidget(const QJsonObject &selector, QString *errorMessage, QJsonArray *candidates)
    {
        refreshRegistry();

        QList<QWidget *> matches;
        const QList<QWidget *> widgets = candidateWidgets();
        for (QWidget *widget : widgets)
        {
            if (widget != nullptr && matchesSelectorImpl(widget, selector))
            {
                matches.append(widget);
            }
        }

        if (matches.isEmpty())
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("Selector did not match any widget.");
            }
            return nullptr;
        }

        if (matches.size() > 1)
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("Selector matched %1 widgets.").arg(matches.size());
            }
            if (candidates != nullptr)
            {
                for (QWidget *widget : matches)
                {
                    candidates->append(widgetSummary(widget));
                }
            }
            return nullptr;
        }

        return matches.first();
    }

    QWidget *chooseWindowTarget(const QJsonObject &selector, QString *errorMessage)
    {
        if (!selector.isEmpty())
        {
            QWidget *widget = findSingleWidget(selector, errorMessage);
            if (widget != nullptr)
            {
                return widget->window();
            }
            return nullptr;
        }

        if (QWidget *activeWindow = QApplication::activeWindow())
        {
            return activeWindow;
        }

        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget *widget : topLevels)
        {
            if (widget != nullptr && widget->isVisible())
            {
                return widget;
            }
        }

        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("No visible window is available.");
        }
        return nullptr;
    }

    QJsonObject widgetSummary(const QWidget *widget)
    {
        refreshRegistry();
        return widgetSummaryImpl(widget);
    }

    QJsonObject styleSummary(const QWidget *widget)
    {
        refreshRegistry();
        return styleSummaryImpl(widget);
    }

    QJsonObject layoutSummary(const QWidget *widget)
    {
        refreshRegistry();
        return layoutSummaryImpl(widget);
    }

    QString refForWidget(const QWidget *widget)
    {
        refreshRegistry();
        return ensureRef(widget);
    }

    QWidget *widgetForRef(const QString &ref)
    {
        return widgetByRefInternal(ref);
    }

    QWidget *activePageWidget()
    {
        return activePageWidgetImpl();
    }

    bool isOnCurrentPage(const QWidget *widget)
    {
        return isOnCurrentPageImpl(widget);
    }

    int indexInParent(const QWidget *widget)
    {
        return indexInParentImpl(widget);
    }

    QWidget *scrollContainerForWidget(const QWidget *widget)
    {
        return scrollContainerForWidgetImpl(widget);
    }

} // namespace WidgetIntrospection
