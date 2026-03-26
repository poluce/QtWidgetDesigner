#include <qtautotest/bridge_client.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace
{

    constexpr const char *kProtocolVersion = "2025-11-25";
    constexpr const char *kServerName = "qt-agent-mcp-server";
    constexpr const char *kServerVersion = "0.1.0";

    constexpr int kMaxInputLineBytes = 10 * 1024 * 1024; // 10 MB
    constexpr int kMaxSelectorFields = 20;
    constexpr int kMaxStringFieldLength = 100 * 1024; // 100 KB
    constexpr int kMaxTextLength = 1 * 1024 * 1024;   // 1 MB
    constexpr int kMaxLogLimit = 10000;

    QJsonObject jsonRpcResponse(const QJsonValue &id, const QJsonObject &result)
    {
        return QJsonObject{
            {"jsonrpc", QStringLiteral("2.0")},
            {"id", id},
            {"result", result},
        };
    }

    QJsonObject jsonRpcError(const QJsonValue &id, int code, const QString &message, const QJsonObject &data = QJsonObject())
    {
        QJsonObject error{
            {"code", code},
            {"message", message},
        };

        if (!data.isEmpty())
        {
            error.insert(QStringLiteral("data"), data);
        }

        return QJsonObject{
            {"jsonrpc", QStringLiteral("2.0")},
            {"id", id},
            {"error", error},
        };
    }

    QJsonObject textContent(const QString &text)
    {
        return QJsonObject{
            {"type", QStringLiteral("text")},
            {"text", text},
        };
    }

    QJsonObject toolResultObject(const QJsonObject &structuredContent, bool isError = false)
    {
        return QJsonObject{
            {"content", QJsonArray{textContent(QString::fromUtf8(QJsonDocument(structuredContent).toJson(QJsonDocument::Indented)))}},
            {"structuredContent", structuredContent},
            {"isError", isError},
        };
    }

    bool validateSelector(const QJsonObject &selector, QString *errorMessage)
    {
        if (selector.size() > kMaxSelectorFields)
        {
            *errorMessage = QStringLiteral("Selector has too many fields (max %1).").arg(kMaxSelectorFields);
            return false;
        }

        for (auto it = selector.begin(); it != selector.end(); ++it)
        {
            if (it.value().isString() && it.value().toString().size() > kMaxStringFieldLength)
            {
                *errorMessage = QStringLiteral("Selector field '%1' exceeds maximum string length.").arg(it.key());
                return false;
            }
        }

        return true;
    }

    QJsonObject objectSchema(const QString &description)
    {
        return QJsonObject{
            {"type", QStringLiteral("object")},
            {"description", description},
            {"additionalProperties", true},
        };
    }

    QJsonObject integerSchema(const QString &description)
    {
        return QJsonObject{
            {"type", QStringLiteral("integer")},
            {"description", description},
        };
    }

    QJsonObject stringSchema(const QString &description)
    {
        return QJsonObject{
            {"type", QStringLiteral("string")},
            {"description", description},
        };
    }

    QJsonObject boolAnnotation(bool readOnlyHint)
    {
        return QJsonObject{
            {"readOnlyHint", readOnlyHint},
        };
    }

    QJsonObject toolDefinition(const QString &name, const QString &description,
                               const QJsonObject &inputSchema, bool readOnlyHint)
    {
        return QJsonObject{
            {"name", name},
            {"description", description},
            {"inputSchema", inputSchema},
            {"annotations", boolAnnotation(readOnlyHint)},
        };
    }

    QJsonArray toolDefinitions()
    {
        QJsonArray tools;

        tools.append(toolDefinition(
            QStringLiteral("qt_describe_ui"),
            QStringLiteral("Read the full visible Qt Widgets UI tree. Call this before exploring actions."),
            QJsonObject{{"type", QStringLiteral("object")}, {"properties", QJsonObject{}}, {"additionalProperties", false}},
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_describe_snapshot"),
            QStringLiteral("Read the active windows, focus widget, active page, modal dialog, and visible subtree."),
            QJsonObject{{"type", QStringLiteral("object")}, {"properties", QJsonObject{}}, {"additionalProperties", false}},
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_describe_object_tree"),
            QStringLiteral("Read the Qt object tree. Supports visibleOnly for compact output."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"visibleOnly", QJsonObject{{"type", QStringLiteral("boolean")}}},
                               }},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_describe_layout_tree"),
            QStringLiteral("Read the Qt layout tree, including layout roles, row/column, spacing, and page-container context."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"visibleOnly", QJsonObject{{"type", QStringLiteral("boolean")}}},
                               }},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_describe_subtree"),
            QStringLiteral("Read a subtree by selector or ref. Can return object or layout tree form."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Supports ref, path, objectName, className, textEquals, textContains, placeholderText, windowObjectName, windowTitleContains, ancestorRef, ancestorObjectName, currentPageOnly, isActiveWindow, visible, enabled, indexInParent."))},
                                   {"layoutTree", QJsonObject{{"type", QStringLiteral("boolean")}}},
                                   {"visibleOnly", QJsonObject{{"type", QStringLiteral("boolean")}}},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_describe_style"),
            QStringLiteral("Read palette colors, styleSheet, and standard style hints for a matched widget."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Supports ref, path, objectName, className, textEquals, textContains, placeholderText, windowObjectName, windowTitleContains, ancestorRef, ancestorObjectName, currentPageOnly, isActiveWindow, visible, enabled, indexInParent."))},
                                   {"includeChildren", QJsonObject{{"type", QStringLiteral("boolean")}}},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_describe_active_page"),
            QStringLiteral("Read the currently active page together with object and layout subtrees."),
            QJsonObject{{"type", QStringLiteral("object")}, {"properties", QJsonObject{}}, {"additionalProperties", false}},
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_list_windows"),
            QStringLiteral("List visible top-level Qt windows and which one is active."),
            QJsonObject{{"type", QStringLiteral("object")}, {"properties", QJsonObject{}}, {"additionalProperties", false}},
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_focus_window"),
            QStringLiteral("Focus a Qt window that matches the selector."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Window selector using objectName, className, textContains, windowTitleContains, visible, enabled."))},
                               }},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_find_widgets"),
            QStringLiteral("Find widgets that match a selector."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector using objectName, className, textContains, windowTitleContains, visible, enabled."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_press_key"),
            QStringLiteral("Send a single key press to a matched widget or the current focus widget."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Optional widget selector."))},
                                   {"key", stringSchema(QStringLiteral("Key name such as Tab, Enter, Escape, Up, Down, A, F5."))},
                                   {"modifiers", stringSchema(QStringLiteral("Optional modifiers string such as Ctrl+Shift."))},
                               }},
                {"required", QJsonArray{QStringLiteral("key")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_send_shortcut"),
            QStringLiteral("Send a shortcut like Ctrl+S or Alt+Enter to the matched widget or focus widget."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Optional widget selector."))},
                                   {"shortcut", stringSchema(QStringLiteral("Shortcut text such as Ctrl+S."))},
                               }},
                {"required", QJsonArray{QStringLiteral("shortcut")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_scroll"),
            QStringLiteral("Scroll a scrollable widget or the nearest scroll container of a matched widget."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                                   {"direction", stringSchema(QStringLiteral("up, down, left, or right."))},
                                   {"amount", integerSchema(QStringLiteral("Scroll amount in pixels or logical units."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector"), QStringLiteral("direction")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_scroll_into_view"),
            QStringLiteral("Scroll a widget into view inside a compatible scroll area."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_select_item"),
            QStringLiteral("Select an item in QListWidget, QTreeWidget, or QTableWidget."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                                   {"options", objectSchema(QStringLiteral("Selection options. Supported keys: text, row, column, path."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector"), QStringLiteral("options")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_toggle_check"),
            QStringLiteral("Set the checked state of a checkable button such as QCheckBox or QRadioButton."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                                   {"checked", QJsonObject{{"type", QStringLiteral("boolean")}}},
                               }},
                {"required", QJsonArray{QStringLiteral("selector"), QStringLiteral("checked")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_choose_combo_option"),
            QStringLiteral("Choose a QComboBox option by text or index."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                                   {"text", stringSchema(QStringLiteral("Option text."))},
                                   {"index", integerSchema(QStringLiteral("Option index."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_activate_tab"),
            QStringLiteral("Activate a QTabWidget or QTabBar tab by text or index."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                                   {"text", stringSchema(QStringLiteral("Tab text."))},
                                   {"index", integerSchema(QStringLiteral("Tab index."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_switch_stacked_page"),
            QStringLiteral("Switch a QStackedWidget page by index."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                                   {"index", integerSchema(QStringLiteral("Page index."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector"), QStringLiteral("index")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_expand_tree_node"),
            QStringLiteral("Expand a QTreeWidget node by path."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                                   {"path", QJsonObject{{"type", QStringLiteral("array")}}},
                               }},
                {"required", QJsonArray{QStringLiteral("selector"), QStringLiteral("path")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_collapse_tree_node"),
            QStringLiteral("Collapse a QTreeWidget node by path."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector."))},
                                   {"path", QJsonObject{{"type", QStringLiteral("array")}}},
                               }},
                {"required", QJsonArray{QStringLiteral("selector"), QStringLiteral("path")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_click"),
            QStringLiteral("Perform a real mouse click on a uniquely matched widget."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector using objectName, className, textContains, windowTitleContains, visible, enabled."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_set_text"),
            QStringLiteral("Focus a uniquely matched editable widget and type text into it."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector using objectName, className, textContains, windowTitleContains, visible, enabled."))},
                                   {"text", stringSchema(QStringLiteral("Text to type into the widget."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector"), QStringLiteral("text")}},
                {"additionalProperties", false},
            },
            false));

        tools.append(toolDefinition(
            QStringLiteral("qt_assert_widget"),
            QStringLiteral("Assert properties of a uniquely matched widget, such as text, visibility, enabled state, or risk level."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector using objectName, className, textContains, windowTitleContains, visible, enabled."))},
                                   {"assertions", objectSchema(QStringLiteral("Assertions object. Supported keys: textEquals, textContains, visible, enabled, objectName, className, windowTitleContains, isInteractive, isInput, riskLevel."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_wait_for_widget"),
            QStringLiteral("Poll until a uniquely matched widget exists and optional assertions pass."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Widget selector using objectName, className, textContains, windowTitleContains, visible, enabled."))},
                                   {"assertions", objectSchema(QStringLiteral("Optional assertions with the same keys as qt_assert_widget."))},
                                   {"timeoutMs", integerSchema(QStringLiteral("Maximum wait time in milliseconds."))},
                                   {"pollIntervalMs", integerSchema(QStringLiteral("Polling interval in milliseconds."))},
                               }},
                {"required", QJsonArray{QStringLiteral("selector")}},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_wait_for_log"),
            QStringLiteral("Poll the Qt application logs until textContains or regex matches an entry."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"textContains", stringSchema(QStringLiteral("Case-insensitive log substring to wait for."))},
                                   {"regex", stringSchema(QStringLiteral("Regular expression to match a log entry."))},
                                   {"timeoutMs", integerSchema(QStringLiteral("Maximum wait time in milliseconds."))},
                                   {"pollIntervalMs", integerSchema(QStringLiteral("Polling interval in milliseconds."))},
                                   {"limit", integerSchema(QStringLiteral("How many recent log entries to scan each poll."))},
                               }},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_get_logs"),
            QStringLiteral("Read recent Qt application logs."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"limit", integerSchema(QStringLiteral("Maximum number of recent log entries to return."))},
                               }},
                {"additionalProperties", false},
            },
            true));

        tools.append(toolDefinition(
            QStringLiteral("qt_capture_window"),
            QStringLiteral("Capture the target Qt window and return a Base64 PNG."),
            QJsonObject{
                {"type", QStringLiteral("object")},
                {"properties", QJsonObject{
                                   {"selector", objectSchema(QStringLiteral("Optional widget or window selector. If omitted, the active window is captured."))},
                               }},
                {"additionalProperties", false},
            },
            true));

        return tools;
    }

    QJsonObject buildBridgeParams(const QString &toolName, const QJsonObject &arguments)
    {
        if (toolName == QStringLiteral("qt_describe_object_tree") ||
            toolName == QStringLiteral("qt_describe_layout_tree"))
        {
            return QJsonObject{
                {"visibleOnly", arguments.value(QStringLiteral("visibleOnly")).toBool(false)},
            };
        }

        if (toolName == QStringLiteral("qt_describe_subtree"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"layoutTree", arguments.value(QStringLiteral("layoutTree")).toBool(false)},
                {"visibleOnly", arguments.value(QStringLiteral("visibleOnly")).toBool(false)},
            };
        }

        if (toolName == QStringLiteral("qt_describe_style"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"includeChildren", arguments.value(QStringLiteral("includeChildren")).toBool(false)},
            };
        }

        if (toolName == QStringLiteral("qt_focus_window") ||
            toolName == QStringLiteral("qt_find_widgets") ||
            toolName == QStringLiteral("qt_click") ||
            toolName == QStringLiteral("qt_capture_window"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
            };
        }

        if (toolName == QStringLiteral("qt_set_text"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"text", arguments.value(QStringLiteral("text")).toString()},
            };
        }

        if (toolName == QStringLiteral("qt_assert_widget"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"assertions", arguments.value(QStringLiteral("assertions")).toObject()},
            };
        }

        if (toolName == QStringLiteral("qt_wait_for_widget"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"assertions", arguments.value(QStringLiteral("assertions")).toObject()},
                {"timeoutMs", arguments.value(QStringLiteral("timeoutMs")).toInt(3000)},
                {"pollIntervalMs", arguments.value(QStringLiteral("pollIntervalMs")).toInt(100)},
            };
        }

        if (toolName == QStringLiteral("qt_wait_for_log"))
        {
            return QJsonObject{
                {"textContains", arguments.value(QStringLiteral("textContains")).toString()},
                {"regex", arguments.value(QStringLiteral("regex")).toString()},
                {"timeoutMs", arguments.value(QStringLiteral("timeoutMs")).toInt(3000)},
                {"pollIntervalMs", arguments.value(QStringLiteral("pollIntervalMs")).toInt(100)},
                {"limit", qMin(arguments.value(QStringLiteral("limit")).toInt(200), kMaxLogLimit)},
            };
        }

        if (toolName == QStringLiteral("qt_get_logs"))
        {
            return QJsonObject{
                {"limit", qMin(arguments.value(QStringLiteral("limit")).toInt(50), kMaxLogLimit)},
            };
        }

        if (toolName == QStringLiteral("qt_press_key"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"key", arguments.value(QStringLiteral("key")).toString()},
                {"modifiers", arguments.value(QStringLiteral("modifiers")).toString()},
            };
        }

        if (toolName == QStringLiteral("qt_send_shortcut"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"shortcut", arguments.value(QStringLiteral("shortcut")).toString()},
            };
        }

        if (toolName == QStringLiteral("qt_scroll"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"direction", arguments.value(QStringLiteral("direction")).toString()},
                {"amount", arguments.value(QStringLiteral("amount")).toInt(120)},
            };
        }

        if (toolName == QStringLiteral("qt_scroll_into_view"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
            };
        }

        if (toolName == QStringLiteral("qt_select_item"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"options", arguments.value(QStringLiteral("options")).toObject()},
            };
        }

        if (toolName == QStringLiteral("qt_toggle_check"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"checked", arguments.value(QStringLiteral("checked")).toBool()},
            };
        }

        if (toolName == QStringLiteral("qt_choose_combo_option"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"text", arguments.value(QStringLiteral("text")).toString()},
                {"index", arguments.value(QStringLiteral("index")).toInt(-1)},
            };
        }

        if (toolName == QStringLiteral("qt_activate_tab"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"text", arguments.value(QStringLiteral("text")).toString()},
                {"index", arguments.value(QStringLiteral("index")).toInt(-1)},
            };
        }

        if (toolName == QStringLiteral("qt_switch_stacked_page"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"index", arguments.value(QStringLiteral("index")).toInt(-1)},
            };
        }

        if (toolName == QStringLiteral("qt_expand_tree_node") ||
            toolName == QStringLiteral("qt_collapse_tree_node"))
        {
            return QJsonObject{
                {"selector", arguments.value(QStringLiteral("selector")).toObject()},
                {"path", arguments.value(QStringLiteral("path")).toArray()},
            };
        }

        return QJsonObject{};
    }

    QString bridgeCommandForTool(const QString &toolName)
    {
        if (toolName == QStringLiteral("qt_describe_ui"))
        {
            return QStringLiteral("describe_ui");
        }
        if (toolName == QStringLiteral("qt_describe_snapshot"))
        {
            return QStringLiteral("describe_snapshot");
        }
        if (toolName == QStringLiteral("qt_describe_object_tree"))
        {
            return QStringLiteral("describe_object_tree");
        }
        if (toolName == QStringLiteral("qt_describe_layout_tree"))
        {
            return QStringLiteral("describe_layout_tree");
        }
        if (toolName == QStringLiteral("qt_describe_subtree"))
        {
            return QStringLiteral("describe_subtree");
        }
        if (toolName == QStringLiteral("qt_describe_style"))
        {
            return QStringLiteral("describe_style");
        }
        if (toolName == QStringLiteral("qt_describe_active_page"))
        {
            return QStringLiteral("describe_active_page");
        }
        if (toolName == QStringLiteral("qt_list_windows"))
        {
            return QStringLiteral("list_windows");
        }
        if (toolName == QStringLiteral("qt_focus_window"))
        {
            return QStringLiteral("focus_window");
        }
        if (toolName == QStringLiteral("qt_find_widgets"))
        {
            return QStringLiteral("find_widgets");
        }
        if (toolName == QStringLiteral("qt_click"))
        {
            return QStringLiteral("click");
        }
        if (toolName == QStringLiteral("qt_set_text"))
        {
            return QStringLiteral("set_text");
        }
        if (toolName == QStringLiteral("qt_press_key"))
        {
            return QStringLiteral("press_key");
        }
        if (toolName == QStringLiteral("qt_send_shortcut"))
        {
            return QStringLiteral("send_shortcut");
        }
        if (toolName == QStringLiteral("qt_scroll"))
        {
            return QStringLiteral("scroll");
        }
        if (toolName == QStringLiteral("qt_scroll_into_view"))
        {
            return QStringLiteral("scroll_into_view");
        }
        if (toolName == QStringLiteral("qt_select_item"))
        {
            return QStringLiteral("select_item");
        }
        if (toolName == QStringLiteral("qt_toggle_check"))
        {
            return QStringLiteral("toggle_check");
        }
        if (toolName == QStringLiteral("qt_choose_combo_option"))
        {
            return QStringLiteral("choose_combo_option");
        }
        if (toolName == QStringLiteral("qt_activate_tab"))
        {
            return QStringLiteral("activate_tab");
        }
        if (toolName == QStringLiteral("qt_switch_stacked_page"))
        {
            return QStringLiteral("switch_stacked_page");
        }
        if (toolName == QStringLiteral("qt_expand_tree_node"))
        {
            return QStringLiteral("expand_tree_node");
        }
        if (toolName == QStringLiteral("qt_collapse_tree_node"))
        {
            return QStringLiteral("collapse_tree_node");
        }
        if (toolName == QStringLiteral("qt_assert_widget"))
        {
            return QStringLiteral("assert_widget");
        }
        if (toolName == QStringLiteral("qt_wait_for_widget"))
        {
            return QStringLiteral("wait_for_widget");
        }
        if (toolName == QStringLiteral("qt_wait_for_log"))
        {
            return QStringLiteral("wait_for_log");
        }
        if (toolName == QStringLiteral("qt_get_logs"))
        {
            return QStringLiteral("get_logs");
        }
        if (toolName == QStringLiteral("qt_capture_window"))
        {
            return QStringLiteral("capture_window");
        }

        return QString();
    }

    QJsonObject callTool(const QString &toolName, const QJsonObject &arguments, const qtautotest::BridgeClient &bridgeClient)
    {
        const QString bridgeCommand = bridgeCommandForTool(toolName);
        if (bridgeCommand.isEmpty())
        {
            return QJsonObject{
                {"ok", false},
                {"toolError", QJsonObject{
                                  {"code", QStringLiteral("unknown_tool")},
                                  {"message", QStringLiteral("Unsupported tool name.")},
                              }},
            };
        }

        if (toolName == QStringLiteral("qt_set_text"))
        {
            const QString text = arguments.value(QStringLiteral("text")).toString();
            if (text.size() > kMaxTextLength)
            {
                return QJsonObject{
                    {"ok", false},
                    {"toolError", QJsonObject{
                                      {"code", QStringLiteral("input_too_large")},
                                      {"message", QStringLiteral("Text input exceeds maximum length.")},
                                      {"maxLength", kMaxTextLength},
                                      {"actualLength", text.size()},
                                  }}};
            }
        }

        const QJsonObject bridgeParams = buildBridgeParams(toolName, arguments);

        if (bridgeParams.contains(QStringLiteral("selector")))
        {
            const QJsonObject selector = bridgeParams.value(QStringLiteral("selector")).toObject();
            QString validationError;
            if (!validateSelector(selector, &validationError))
            {
                return QJsonObject{
                    {"ok", false},
                    {"toolError", QJsonObject{
                                      {"code", QStringLiteral("invalid_selector")},
                                      {"message", validationError},
                                  }},
                };
            }
        }

        const qtautotest::BridgeCallResult bridgeResult = bridgeClient.call(bridgeCommand, bridgeParams);
        if (!bridgeResult.transportOk)
        {
            return QJsonObject{
                {"ok", false},
                {"toolError", QJsonObject{
                                  {"code", QStringLiteral("bridge_transport_error")},
                                  {"message", bridgeResult.transportError},
                                  {"bridgeUrl", bridgeClient.bridgeUrl().toString()},
                              }},
            };
        }

        if (!bridgeResult.response.value(QStringLiteral("ok")).toBool())
        {
            return QJsonObject{
                {"ok", false},
                {"bridgeError", bridgeResult.response.value(QStringLiteral("error")).toObject()},
            };
        }

        return QJsonObject{
            {"ok", true},
            {"bridgeResult", bridgeResult.response.value(QStringLiteral("result")).toObject()},
        };
    }

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("QtAgentMcpServer"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(kServerVersion));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption bridgeUrlOption(
        QStringList{QStringLiteral("bridge-url")},
        QStringLiteral("Bridge WebSocket URL."),
        QStringLiteral("url"),
        QStringLiteral("ws://127.0.0.1:49555"));
    parser.addOption(bridgeUrlOption);
    parser.process(app);

    qtautotest::BridgeClient bridgeClient(QUrl(parser.value(bridgeUrlOption)));

    QTextStream input(stdin, QIODevice::ReadOnly);
    QTextStream output(stdout, QIODevice::WriteOnly);
    QTextStream error(stderr, QIODevice::WriteOnly);

    bool initializeHandled = false;

    while (true)
    {
        const QString line = input.readLine();
        if (line.isNull())
        {
            break;
        }

        if (line.trimmed().isEmpty())
        {
            continue;
        }

        if (line.size() > kMaxInputLineBytes)
        {
            error << "Input line exceeds maximum allowed size (" << kMaxInputLineBytes << " bytes)" << Qt::endl;
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            error << "Invalid MCP input: " << parseError.errorString() << Qt::endl;
            continue;
        }

        const QJsonObject request = document.object();
        const QString method = request.value(QStringLiteral("method")).toString();
        const QJsonValue id = request.value(QStringLiteral("id"));
        const bool hasId = request.contains(QStringLiteral("id"));
        const QJsonObject params = request.value(QStringLiteral("params")).toObject();

        if (method.isEmpty())
        {
            if (hasId)
            {
                output << QString::fromUtf8(QJsonDocument(
                                                jsonRpcError(id, -32600, QStringLiteral("Request is missing method.")))
                                                .toJson(QJsonDocument::Compact))
                       << Qt::endl;
                output.flush();
            }
            continue;
        }

        if (method == QStringLiteral("notifications/initialized"))
        {
            continue;
        }

        if (method == QStringLiteral("initialize"))
        {
            initializeHandled = true;

            const QString requestedVersion = params.value(QStringLiteral("protocolVersion")).toString();
            const QString serverVersion = QString::fromLatin1(kProtocolVersion);
            const QString negotiatedVersion =
                (!requestedVersion.isEmpty() && requestedVersion <= serverVersion)
                    ? requestedVersion
                    : serverVersion;

            const QJsonObject result{
                {"protocolVersion", negotiatedVersion},
                {"capabilities", QJsonObject{
                                     {"tools", QJsonObject{
                                                   {"listChanged", false},
                                               }},
                                 }},
                {"serverInfo", QJsonObject{
                                   {"name", QString::fromLatin1(kServerName)},
                                   {"version", QString::fromLatin1(kServerVersion)},
                               }},
                {"instructions", QStringLiteral("Use qt_describe_ui before actions. Prefer low-risk read tools first, then click/set_text, then assert or wait tools to verify outcomes.")},
            };

            output << QString::fromUtf8(QJsonDocument(jsonRpcResponse(id, result)).toJson(QJsonDocument::Compact))
                   << Qt::endl;
            output.flush();
            continue;
        }

        if (!initializeHandled)
        {
            if (hasId)
            {
                output << QString::fromUtf8(QJsonDocument(
                                                jsonRpcError(id, -32002, QStringLiteral("Server not initialized.")))
                                                .toJson(QJsonDocument::Compact))
                       << Qt::endl;
                output.flush();
            }
            continue;
        }

        if (method == QStringLiteral("ping"))
        {
            if (hasId)
            {
                output << QString::fromUtf8(QJsonDocument(jsonRpcResponse(id, QJsonObject{})).toJson(QJsonDocument::Compact))
                       << Qt::endl;
                output.flush();
            }
            continue;
        }

        if (method == QStringLiteral("tools/list"))
        {
            const QJsonObject result{
                {"tools", toolDefinitions()},
            };

            output << QString::fromUtf8(QJsonDocument(jsonRpcResponse(id, result)).toJson(QJsonDocument::Compact))
                   << Qt::endl;
            output.flush();
            continue;
        }

        if (method == QStringLiteral("tools/call"))
        {
            const QString toolName = params.value(QStringLiteral("name")).toString();
            const QJsonObject arguments = params.value(QStringLiteral("arguments")).toObject();
            const QJsonObject toolCallResult = callTool(toolName, arguments, bridgeClient);

            const bool isError = !toolCallResult.value(QStringLiteral("ok")).toBool();
            QJsonObject structuredContent = toolCallResult;
            structuredContent.remove(QStringLiteral("ok"));

            output << QString::fromUtf8(QJsonDocument(
                                            jsonRpcResponse(id, toolResultObject(structuredContent, isError)))
                                            .toJson(QJsonDocument::Compact))
                   << Qt::endl;
            output.flush();
            continue;
        }

        if (hasId)
        {
            output << QString::fromUtf8(QJsonDocument(
                                            jsonRpcError(id, -32601, QStringLiteral("Method not found.")))
                                            .toJson(QJsonDocument::Compact))
                   << Qt::endl;
            output.flush();
        }
    }

    return 0;
}
