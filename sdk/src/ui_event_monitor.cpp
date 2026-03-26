#include "ui_event_monitor.h"

#include "widget_introspection.h"

#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QSet>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include <QWindow>

namespace
{

    QJsonObject widgetSummaryOrEmpty(QWidget *widget)
    {
        return widget != nullptr ? WidgetIntrospection::widgetSummary(widget) : QJsonObject{};
    }

    QStringList collectRefs(const QList<QWidget *> &widgets)
    {
        QStringList refs;
        for (QWidget *widget : widgets)
        {
            if (widget == nullptr)
            {
                continue;
            }

            const QString ref = WidgetIntrospection::refForWidget(widget);
            if (!ref.isEmpty() && !refs.contains(ref))
            {
                refs.append(ref);
            }
        }
        return refs;
    }

    QString tabTextAt(const QTabWidget *tabWidget, int index)
    {
        if (tabWidget == nullptr || index < 0 || index >= tabWidget->count())
        {
            return QString();
        }
        return tabWidget->tabText(index);
    }

    QWidget *pageAt(QTabWidget *tabWidget, int index)
    {
        if (tabWidget == nullptr || index < 0 || index >= tabWidget->count())
        {
            return nullptr;
        }
        return tabWidget->widget(index);
    }

    QWidget *pageAt(QStackedWidget *stackedWidget, int index)
    {
        if (stackedWidget == nullptr || index < 0 || index >= stackedWidget->count())
        {
            return nullptr;
        }
        return stackedWidget->widget(index);
    }

} // namespace

UiEventMonitor::UiEventMonitor(QObject *parent)
    : QObject(parent)
{
    qApp->installEventFilter(this);

    m_lastActiveWindow = QApplication::activeWindow();
    m_lastFocusWidget = QApplication::focusWidget();
    m_lastModalDialog = QApplication::activeModalWidget();

    QObject::connect(qApp, &QApplication::focusChanged, this, &UiEventMonitor::handleFocusChanged);
    QObject::connect(qApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow *)
                     {
        checkActiveWindowChanged();
        scheduleStateRefresh(); });

    scanWidgets();
}

bool UiEventMonitor::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::ChildAdded:
    case QEvent::Show:
    case QEvent::Hide:
    case QEvent::Close:
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::Destroy:
        scheduleStateRefresh();
        break;
    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}

void UiEventMonitor::scanWidgets()
{
    const auto widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets)
    {
        trackWidget(widget);
    }
}

void UiEventMonitor::trackWidget(QWidget *widget)
{
    if (widget == nullptr)
    {
        return;
    }

    if (auto *tabWidget = qobject_cast<QTabWidget *>(widget))
    {
        trackTabWidget(tabWidget);
    }

    if (auto *stackedWidget = qobject_cast<QStackedWidget *>(widget))
    {
        trackStackedWidget(stackedWidget);
    }
}

void UiEventMonitor::trackTabWidget(QTabWidget *tabWidget)
{
    if (tabWidget == nullptr || m_trackedTabWidgets.contains(tabWidget))
    {
        return;
    }

    m_trackedTabWidgets.insert(tabWidget);
    m_lastIndexByContainer.insert(tabWidget, tabWidget->currentIndex());

    QObject::connect(tabWidget, &QTabWidget::currentChanged, this,
                     [this, weakTab = QPointer<QTabWidget>(tabWidget)](int index)
                     {
                         if (weakTab != nullptr)
                         {
                             emitTabChanged(weakTab, index);
                         }
                     });

    QObject::connect(tabWidget, &QObject::destroyed, this, [this, tabWidget]()
                     {
        m_trackedTabWidgets.remove(tabWidget);
        m_lastIndexByContainer.remove(tabWidget); });
}

void UiEventMonitor::trackStackedWidget(QStackedWidget *stackedWidget)
{
    if (stackedWidget == nullptr || m_trackedStackedWidgets.contains(stackedWidget))
    {
        return;
    }

    m_trackedStackedWidgets.insert(stackedWidget);
    m_lastIndexByContainer.insert(stackedWidget, stackedWidget->currentIndex());

    QObject::connect(stackedWidget, &QStackedWidget::currentChanged, this,
                     [this, weakStacked = QPointer<QStackedWidget>(stackedWidget)](int index)
                     {
                         if (weakStacked == nullptr)
                         {
                             return;
                         }
                         const int previousIndex = m_lastIndexByContainer.value(weakStacked, -1);
                         m_lastIndexByContainer.insert(weakStacked, index);
                         emitActivePageChanged(weakStacked, index, previousIndex);
                     });

    QObject::connect(stackedWidget, &QObject::destroyed, this, [this, stackedWidget]()
                     {
        m_trackedStackedWidgets.remove(stackedWidget);
        m_lastIndexByContainer.remove(stackedWidget); });
}

void UiEventMonitor::emitTabChanged(QTabWidget *tabWidget, int index)
{
    const int previousIndex = m_lastIndexByContainer.value(tabWidget, -1);
    const QString previousText = tabTextAt(tabWidget, previousIndex);
    QWidget *previousPage = pageAt(tabWidget, previousIndex);
    QWidget *currentPage = pageAt(tabWidget, index);

    m_lastIndexByContainer.insert(tabWidget, index);

    QJsonObject payload{
        {"widget", widgetSummaryOrEmpty(tabWidget)},
        {"index", index},
        {"previousIndex", previousIndex},
        {"text", tabTextAt(tabWidget, index)},
        {"previousText", previousText},
        {"page", widgetSummaryOrEmpty(currentPage)},
        {"previousPage", widgetSummaryOrEmpty(previousPage)},
        {"reason", QStringLiteral("currentChanged")},
    };

    emitEvent(QStringLiteral("tab_changed"), payload,
              QList<QWidget *>{tabWidget, currentPage, previousPage, tabWidget != nullptr ? tabWidget->window() : nullptr});

    emitActivePageChanged(tabWidget, index, previousIndex);
}

void UiEventMonitor::emitActivePageChanged(QWidget *container, int index, int previousIndex)
{
    QWidget *currentPage = nullptr;
    if (auto *tabWidget = qobject_cast<QTabWidget *>(container))
    {
        currentPage = pageAt(tabWidget, index);
    }
    else if (auto *stackedWidget = qobject_cast<QStackedWidget *>(container))
    {
        currentPage = pageAt(stackedWidget, index);
    }

    QJsonObject payload{
        {"container", widgetSummaryOrEmpty(container)},
        {"containerType", container != nullptr ? QString::fromUtf8(container->metaObject()->className()) : QString()},
        {"index", index},
        {"previousIndex", previousIndex},
        {"page", widgetSummaryOrEmpty(currentPage)},
        {"pageContext", pageContext()},
    };

    emitEvent(QStringLiteral("active_page_changed"), payload,
              QList<QWidget *>{container, currentPage, container != nullptr ? container->window() : nullptr});
}

void UiEventMonitor::emitEvent(const QString &eventName, const QJsonObject &payload, const QList<QWidget *> &relatedWidgets)
{
    emit eventObserved(eventName, payload, collectRefs(relatedWidgets));
}

void UiEventMonitor::handleFocusChanged(QWidget *previous, QWidget *current)
{
    if (previous == current)
    {
        return;
    }

    m_lastFocusWidget = current;

    QWidget *owningWindow = current != nullptr ? current->window() : QApplication::activeWindow();
    QJsonObject payload{
        {"widget", widgetSummaryOrEmpty(current)},
        {"previousWidget", widgetSummaryOrEmpty(previous)},
        {"window", widgetSummaryOrEmpty(owningWindow)},
        {"focusReason", QString()},
    };

    emitEvent(QStringLiteral("focus_widget_changed"), payload,
              QList<QWidget *>{current, previous, owningWindow});

    scheduleStateRefresh();
}

void UiEventMonitor::checkActiveWindowChanged()
{
    QWidget *currentWindow = QApplication::activeWindow();
    if (currentWindow == m_lastActiveWindow)
    {
        return;
    }

    QWidget *previousWindow = m_lastActiveWindow;
    m_lastActiveWindow = currentWindow;

    QJsonObject payload{
        {"window", widgetSummaryOrEmpty(currentWindow)},
        {"previousWindow", widgetSummaryOrEmpty(previousWindow)},
        {"windowTitle", currentWindow != nullptr ? currentWindow->windowTitle() : QString()},
        {"objectName", currentWindow != nullptr ? currentWindow->objectName() : QString()},
    };

    emitEvent(QStringLiteral("window_focus_changed"), payload,
              QList<QWidget *>{currentWindow, previousWindow});
}

void UiEventMonitor::checkActiveModalDialogChanged()
{
    QWidget *currentDialog = QApplication::activeModalWidget();
    if (currentDialog == m_lastModalDialog)
    {
        return;
    }

    QWidget *previousDialog = m_lastModalDialog;
    m_lastModalDialog = currentDialog;

    if (previousDialog != nullptr)
    {
        QJsonObject payload{
            {"dialog", widgetSummaryOrEmpty(previousDialog)},
            {"state", QStringLiteral("closed")},
            {"window", widgetSummaryOrEmpty(previousDialog->window())},
            {"title", previousDialog->windowTitle()},
        };
        emitEvent(QStringLiteral("modal_dialog_changed"), payload,
                  QList<QWidget *>{previousDialog, previousDialog->window()});
    }

    if (currentDialog != nullptr)
    {
        QJsonObject payload{
            {"dialog", widgetSummaryOrEmpty(currentDialog)},
            {"state", QStringLiteral("opened")},
            {"window", widgetSummaryOrEmpty(currentDialog->window())},
            {"title", currentDialog->windowTitle()},
        };
        emitEvent(QStringLiteral("modal_dialog_changed"), payload,
                  QList<QWidget *>{currentDialog, currentDialog->window()});
    }
}

void UiEventMonitor::scheduleStateRefresh()
{
    if (m_stateRefreshScheduled)
    {
        return;
    }

    m_stateRefreshScheduled = true;
    QTimer::singleShot(0, this, &UiEventMonitor::refreshDynamicState);
}

void UiEventMonitor::refreshDynamicState()
{
    m_stateRefreshScheduled = false;
    scanWidgets();
    checkActiveWindowChanged();
    checkActiveModalDialogChanged();
}

QJsonObject UiEventMonitor::pageContext() const
{
    QJsonObject context;

    if (QWidget *activeWindow = QApplication::activeWindow())
    {
        context.insert(QStringLiteral("activeWindow"), WidgetIntrospection::widgetSummary(activeWindow));
    }

    if (QWidget *focusWidget = QApplication::focusWidget())
    {
        context.insert(QStringLiteral("focusWidget"), WidgetIntrospection::widgetSummary(focusWidget));
    }

    if (QWidget *modalDialog = QApplication::activeModalWidget())
    {
        context.insert(QStringLiteral("modalDialog"), WidgetIntrospection::widgetSummary(modalDialog));
    }

    return context;
}
