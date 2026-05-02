#pragma once

#include <QImage>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include <memory>
#include <vector>

namespace qtautotest {

struct IntRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct BoxMargins
{
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

struct ColorValue
{
    bool valid = false;
    QString hex;
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 255;
};

struct WidgetInfo
{
    QString ref;
    QString objectName;
    QString className;
    QString text;
    QString placeholderText;
    QString windowTitle;
    QString path;
    QString pageContainerType;
    QString riskLevel;
    bool visible = false;
    bool enabled = false;
    bool interactive = false;
    bool input = false;
    bool container = false;
    bool currentPage = false;
    bool hasFocus = false;
    bool activeWindow = false;
    int depth = 0;
    int indexInParent = -1;
    IntRect geometry;
    QStringList availableActions;

    bool isValid() const
    {
        return !ref.isEmpty() || !objectName.isEmpty() || !className.isEmpty();
    }
};

struct SnapshotNode : WidgetInfo
{
    std::vector<std::shared_ptr<SnapshotNode>> children;
};

struct WindowInfo : WidgetInfo
{
    bool active = false;
    bool modal = false;
};

struct LayoutNode
{
    QString ref;
    QString ownerRef;
    QString objectName;
    QString className;
    QString layoutType;
    QString layoutRole;
    QString alignment;
    QString pageContainerType;
    int indexInParent = -1;
    int row = -1;
    int column = -1;
    int rowSpan = 1;
    int columnSpan = 1;
    int stretch = -1;
    BoxMargins margins;
    int spacing = -1;
    bool scrollArea = false;
    bool currentPage = false;
    std::vector<std::shared_ptr<LayoutNode>> children;

    bool isValid() const
    {
        return !ref.isEmpty() || !ownerRef.isEmpty() || !className.isEmpty();
    }
};

struct PaletteGroup
{
    QMap<QString, ColorValue> colors;
};

struct StyleRoles
{
    QString foregroundRole;
    ColorValue foregroundColor;
    QString backgroundRole;
    ColorValue backgroundColor;
};

struct StyleAttachmentView
{
    WidgetInfo widget;
    QString styleSheet;
    bool inheritsStyleSheet = false;
    bool autoFillBackground = false;
    QString currentColorGroup;
    StyleRoles roles;
    PaletteGroup currentPalette;
    PaletteGroup activePalette;
    PaletteGroup inactivePalette;
    PaletteGroup disabledPalette;
    QMap<QString, ColorValue> classHints;

    bool isValid() const
    {
        return widget.isValid();
    }
};

struct StyleView
{
    QString styleSheet;
    bool inheritsStyleSheet = false;
    bool autoFillBackground = false;
    QString currentColorGroup;
    StyleRoles roles;
    PaletteGroup currentPalette;
    PaletteGroup activePalette;
    PaletteGroup inactivePalette;
    PaletteGroup disabledPalette;
    QMap<QString, ColorValue> classHints;
    StyleAttachmentView viewport;
    bool hasViewport = false;
    StyleAttachmentView header;
    bool hasHeader = false;
    StyleAttachmentView horizontalHeader;
    bool hasHorizontalHeader = false;
    StyleAttachmentView verticalHeader;
    bool hasVerticalHeader = false;
};

struct StyleTreeNode
{
    WidgetInfo widget;
    StyleView style;
    std::vector<std::shared_ptr<StyleTreeNode>> children;
};

struct StyleTreeView
{
    bool includeChildren = false;
    StyleTreeNode root;
};

struct SnapshotView
{
    QVector<SnapshotNode> windows;
    QVector<WindowInfo> topLevelWindows;
    int widgetCount = 0;

    SnapshotNode focusWidget;
    bool hasFocusWidget = false;

    SnapshotNode activeWindow;
    bool hasActiveWindow = false;

    SnapshotNode visibleSubtree;
    bool hasVisibleSubtree = false;

    SnapshotNode modalDialog;
    bool hasModalDialog = false;

    SnapshotNode activePage;
    bool hasActivePage = false;
};

struct ObjectTreeView
{
    bool visibleOnly = false;
    QVector<SnapshotNode> windows;
};

struct LayoutTreeView
{
    bool visibleOnly = false;
    QVector<LayoutNode> windows;
};

enum class TreeKind
{
    Unknown,
    Object,
    Layout,
    Style,
};

struct SubtreeView
{
    TreeKind treeKind = TreeKind::Unknown;
    bool visibleOnly = false;
    SnapshotNode objectRoot;
    bool hasObjectRoot = false;
    LayoutNode layoutRoot;
    bool hasLayoutRoot = false;
};

struct ActivePageView
{
    SnapshotNode page;
    SnapshotNode objectTree;
    LayoutNode layoutTree;
};

struct WindowCapture
{
    SnapshotNode window;
    QImage image;
};

} // namespace qtautotest
