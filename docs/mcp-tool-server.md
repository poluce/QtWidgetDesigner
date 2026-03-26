# Qt MCP Tool Server

`QtAgentMcpServer` 是一个独立的 MCP stdio server。

它的职责只有一件事：把 `QtAgentAutotest` 暴露出的桥接命令映射成 MCP tools，供外部 LLM/Agent 调用。

它不内置模型，不负责测试规划，也不替外部 LLM 决定下一步做什么。

## 架构关系

```text
External LLM / Agent
        |
        v
QtAgentMcpServer (MCP stdio tools)
        |
        v
QtAgentAutotest Agent Bridge (WebSocket JSON)
        |
        v
Qt Widgets Application
```

## 启动顺序

1. 启动 Qt 应用

```powershell
./build/QtAgentAutotest.exe --port 49555
```

如果要做对人类用户友好的可视化演示，可以改成：

```powershell
./build/QtAgentAutotest.exe --port 49555 --demo-visible
```

2. 启动 MCP server

```powershell
./build/QtAgentMcpServer.exe --bridge-url ws://127.0.0.1:49555
```

3. 让外部 LLM 通过 MCP 连接 `QtAgentMcpServer`

## 暴露的 MCP tools

- `qt_describe_ui`
- `qt_describe_snapshot`
- `qt_describe_object_tree`
- `qt_describe_layout_tree`
- `qt_describe_subtree`
- `qt_describe_style`
- `qt_describe_active_page`
- `qt_list_windows`
- `qt_focus_window`
- `qt_find_widgets`
- `qt_click`
- `qt_set_text`
- `qt_press_key`
- `qt_send_shortcut`
- `qt_scroll`
- `qt_scroll_into_view`
- `qt_select_item`
- `qt_toggle_check`
- `qt_choose_combo_option`
- `qt_activate_tab`
- `qt_switch_stacked_page`
- `qt_expand_tree_node`
- `qt_collapse_tree_node`
- `qt_assert_widget`
- `qt_wait_for_widget`
- `qt_wait_for_log`
- `qt_get_logs`
- `qt_capture_window`

## 设计原则

- MCP tool 名与桥接命令尽量一一对应，减少隐藏逻辑。
- 快照优先返回结构化树，而不是先依赖截图。
- 颜色与样式优先返回 palette / styleSheet 摘要，而不是先依赖截图采样。
- `ref` 是首选锚点，selector 是兜底。
- 成功时返回 `bridgeResult`。
- 桥接失败时返回 `bridgeError`。
- 连接桥接失败时返回 `toolError`。
- 外部 LLM 应先调用 `qt_describe_snapshot` 或 `qt_describe_object_tree`，再决定后续动作。

## 适合的外部 LLM 使用方式

### 自主探索测试

外部 LLM 使用：

1. `qt_describe_snapshot`
2. `qt_describe_object_tree` / `qt_describe_layout_tree`
3. 必要时 `qt_describe_style`
4. 从结果中读取稳定 `ref`
5. `qt_click` / `qt_set_text` / `qt_press_key` / `qt_scroll` / `qt_select_item`
6. `qt_assert_widget` / `qt_wait_for_widget` / `qt_wait_for_log`

更推荐的新流程是：

1. `qt_describe_snapshot`
2. 从树中读取 `ref`
3. 后续动作优先按 `ref` 调用
4. 使用 `qt_wait_for_widget` / `qt_assert_widget` 验证结果

## 当前已覆盖的复杂组件

- `QTabWidget` / `QTabBar`
- `QStackedWidget`
- `QListWidget`
- `QTreeWidget`
- `QTableWidget`
- `QComboBox`
- `QCheckBox` / `QRadioButton`
- `QScrollArea`
- `QDialog`

### 目标驱动测试

用户只说：

```text
测试登录流程和计数器
```

外部 LLM 自己拆解成：

1. 读取 UI 树
2. 找到登录相关输入框和按钮
3. 输入文本
4. 点击登录
5. 校验状态或日志
6. 找到计数器按钮
7. 点击并断言 `Counter: 1`

Qt 侧只提供稳定工具，不提供模型。
