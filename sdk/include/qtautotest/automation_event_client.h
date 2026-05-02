#pragma once

#include <qtautotest/automation_client.h>

#include <QUrl>

#include <functional>

namespace qtautotest {

struct SubscriptionHandle
{
    QString id;

    bool isValid() const { return !id.isEmpty(); }
};

struct EventSubscriptionSpec
{
    QVector<EventKind> events;
    QVector<Selector> selectors;
    int debounceMs = 50;
};

struct PageContext
{
    SnapshotNode activeWindow;
    bool hasActiveWindow = false;
    SnapshotNode focusWidget;
    bool hasFocusWidget = false;
    SnapshotNode modalDialog;
    bool hasModalDialog = false;
};

struct TabChangedEvent
{
    SubscriptionHandle subscription;
    qint64 sequence = 0;
    SnapshotNode widget;
    int index = -1;
    int previousIndex = -1;
    QString text;
    QString previousText;
    SnapshotNode page;
    bool hasPage = false;
    SnapshotNode previousPage;
    bool hasPreviousPage = false;
    QString reason;
};

struct ActivePageChangedEvent
{
    SubscriptionHandle subscription;
    qint64 sequence = 0;
    SnapshotNode container;
    QString containerType;
    int index = -1;
    int previousIndex = -1;
    SnapshotNode page;
    bool hasPage = false;
    PageContext context;
};

struct WindowFocusChangedEvent
{
    SubscriptionHandle subscription;
    qint64 sequence = 0;
    SnapshotNode window;
    bool hasWindow = false;
    SnapshotNode previousWindow;
    bool hasPreviousWindow = false;
    QString windowTitle;
    QString objectName;
};

struct FocusWidgetChangedEvent
{
    SubscriptionHandle subscription;
    qint64 sequence = 0;
    SnapshotNode widget;
    bool hasWidget = false;
    SnapshotNode previousWidget;
    bool hasPreviousWidget = false;
    SnapshotNode window;
    bool hasWindow = false;
    QString focusReason;
};

enum class ModalDialogState
{
    Unknown,
    Opened,
    Closed,
};

struct ModalDialogChangedEvent
{
    SubscriptionHandle subscription;
    qint64 sequence = 0;
    SnapshotNode dialog;
    bool hasDialog = false;
    ModalDialogState state = ModalDialogState::Unknown;
    SnapshotNode window;
    bool hasWindow = false;
    QString title;
};

class AutomationEventClient
{
public:
    AutomationEventClient();
    explicit AutomationEventClient(QUrl bridgeUrl);
    ~AutomationEventClient();

    const QUrl& bridgeUrl() const;
    void setBridgeUrl(QUrl bridgeUrl);

    OperationResult connectToBridge(int timeoutMs = 5000);
    void disconnectFromBridge();
    bool isConnected() const;
    QString errorString() const;

    Result<QVector<EventKind>> supportedEvents(int timeoutMs = 5000);
    Result<SubscriptionHandle> subscribe(const EventSubscriptionSpec& spec, int timeoutMs = 5000);
    OperationResult unsubscribe(const SubscriptionHandle& handle, int timeoutMs = 5000);

    void setConnectedHandler(std::function<void()> handler);
    void setDisconnectedHandler(std::function<void()> handler);
    void setTransportErrorHandler(std::function<void(const QString&)> handler);
    void setTabChangedHandler(std::function<void(const TabChangedEvent&)> handler);
    void setActivePageChangedHandler(std::function<void(const ActivePageChangedEvent&)> handler);
    void setWindowFocusChangedHandler(std::function<void(const WindowFocusChangedEvent&)> handler);
    void setFocusWidgetChangedHandler(std::function<void(const FocusWidgetChangedEvent&)> handler);
    void setModalDialogChangedHandler(std::function<void(const ModalDialogChangedEvent&)> handler);

private:
    class Impl;
    Impl* m_impl = nullptr;
};

} // namespace qtautotest
