# Agent Bridge Protocol

`QtAgentAutotest` 会在本地启动一个 WebSocket 服务：

- 地址：`ws://127.0.0.1:49555`
- 消息格式：UTF-8 JSON
- 一次请求对应一次响应

这层协议是“Qt 应用内桥接层”，不包含 LLM。外部 LLM 可以直接连它，也可以更推荐地通过独立的 MCP server 调用它。

## 请求格式

```json
{
  "id": "req-001",
  "command": "click",
  "selector": {
    "objectName": "incrementButton"
  }
}
```

## 响应格式

成功：

```json
{
  "id": "req-001",
  "ok": true,
  "result": {
    "clicked": "incrementButton"
  }
}
```

失败：

```json
{
  "id": "req-001",
  "ok": false,
  "error": {
    "code": "ambiguous_selector",
    "message": "Selector matched 2 widgets."
  }
}
```

## Selector

当前支持的 selector 字段：

- `ref`
- `path`
- `objectName`
- `className`
- `textEquals`
- `textContains`
- `placeholderText`
- `windowObjectName`
- `windowTitleContains`
- `ancestorRef`
- `ancestorObjectName`
- `currentPageOnly`
- `isActiveWindow`
- `visible`
- `enabled`
- `indexInParent`

建议优先给关键控件设置稳定的 `objectName`，让外部 LLM 的探索更可靠。
如果已经拿到 `ref`，优先按 `ref` 操作。

## 快照输出要点

当前桥接层支持三类核心快照：

- `describe_snapshot`
- `describe_object_tree`
- `describe_layout_tree`

其中对象节点至少包含：

- `ref`
- `objectName`
- `className`
- `text`
- `placeholderText`
- `windowTitle`
- `visible`
- `enabled`
- `path`
- `depth`
- `isInteractive`
- `isInput`
- `isContainer`
- `riskLevel`
- `geometry`
- `availableActions`
- `children`

样式读取命令 `describe_style` 返回的样式节点至少包含：

- `widget`
- `style.styleSheet`
- `style.inheritsStyleSheet`
- `style.autoFillBackground`
- `style.currentColorGroup`
- `style.roles`
- `style.palette`
- `style.classHints`

其中：

- `path` 便于外部 LLM 在多次观察时稳定引用控件
- `ref` 是会话内稳定锚点，推荐优先用于动作执行
- `riskLevel` 当前只区分 `safe_read`、`safe_action`、`unknown`
- `availableActions` 说明该控件当前允许的动作，例如 `click`、`set_text`

布局树节点至少包含：

- `ref`
- `ownerRef`
- `layoutType`
- `layoutRole`
- `indexInParent`
- `row`
- `column`
- `stretch`
- `alignment`
- `margins`
- `spacing`
- `isScrollArea`
- `isCurrentPage`
- `pageContainerType`
- `children`

## Commands

### `ping`

用于连通性检查。

### `list_commands`

返回当前桥接层支持的命令名列表。

### `describe_ui`

兼容命令。内部等价于 `describe_snapshot`。

### `describe_snapshot`

返回当前活动窗口、焦点控件、活动页面、模态对话框与可见子树。

### `describe_object_tree`

返回对象树，支持参数：

- `visibleOnly`

### `describe_layout_tree`

返回布局树，支持参数：

- `visibleOnly`

### `describe_subtree`

按 selector 读取子树，支持参数：

- `selector`
- `layoutTree`
- `visibleOnly`

### `describe_style`

按 selector 读取单个控件的 palette 颜色、styleSheet 与标准样式摘要，支持参数：

- `selector`
- `includeChildren`

返回重点：

- `styleSheet`
  当前控件自身的 styleSheet 原文
- `inheritsStyleSheet`
  当前控件自身没有 styleSheet，但祖先存在 styleSheet
- `palette`
  当前 / active / inactive / disabled 四组逻辑颜色
- `roles`
  `foregroundRole` / `backgroundRole` 以及解析后的颜色
- `classHints`
  针对标准控件补充的语义颜色，例如按钮填充色、文本色、输入框底色、高亮色

这不是截图采样，不保证与最终像素颜色完全一致；它描述的是 Qt 当前可稳定读取的逻辑样式信息。

### `describe_active_page`

返回当前活动页面以及它对应的对象树和布局树。

### `list_windows`

返回可见顶层窗口列表，并标记哪个窗口处于活动状态。

### `focus_window`

让匹配到的窗口成为活动窗口。

请求：

```json
{
  "id": "1",
  "command": "focus_window",
  "selector": {
    "objectName": "mainWindow"
  }
}
```

### `find_widgets`

返回匹配 selector 的控件列表。

### `click`

对唯一匹配的控件执行真实鼠标点击。

### `set_text`

对唯一匹配的可编辑文本控件执行真实聚焦和键盘输入。

### `press_key`

发送单个按键到目标控件或当前焦点控件。

### `send_shortcut`

发送快捷键，例如 `Ctrl+A`、`Ctrl+S`。

### `scroll`

滚动滚动容器或目标控件所属的最近滚动容器。

### `scroll_into_view`

将目标控件滚动到可见区域。

### `select_item`

对 `QListWidget`、`QTreeWidget`、`QTableWidget` 进行选择。

### `toggle_check`

设置复选框/单选按钮等 checkable 控件状态。

### `choose_combo_option`

按文本或索引选择 `QComboBox` 项。

### `activate_tab`

按文本或索引切换 `QTabWidget` / `QTabBar`。

### `switch_stacked_page`

切换 `QStackedWidget` 页面。

### `expand_tree_node`

按路径展开树节点。

### `collapse_tree_node`

按路径折叠树节点。

### `assert_widget`

对唯一匹配的控件做断言。

支持的断言字段：

- `textEquals`
- `textContains`
- `visible`
- `enabled`
- `objectName`
- `className`
- `windowTitleContains`
- `isInteractive`
- `isInput`
- `riskLevel`

请求：

```json
{
  "id": "2",
  "command": "assert_widget",
  "selector": {
    "objectName": "counterLabel"
  },
  "assertions": {
    "textEquals": "Counter: 1"
  }
}
```

### `wait_for_widget`

轮询等待控件存在，并且可选断言满足。

支持字段：

- `selector`
- `assertions`
- `timeoutMs`
- `pollIntervalMs`

### `wait_for_log`

轮询等待最近日志中出现目标内容。

支持字段：

- `textContains`
- `regex`
- `timeoutMs`
- `pollIntervalMs`
- `limit`

### `get_logs`

读取最近的 Qt 应用日志。

### `capture_window`

抓取目标窗口，返回 Base64 PNG。

如果未提供 selector，则优先抓取活动窗口。

## 推荐的外部 LLM 工作流

1. `describe_ui`
2. `find_widgets`
3. `click` / `set_text`
4. `assert_widget` / `wait_for_widget` / `wait_for_log`
5. 必要时 `capture_window`
6. 再次 `describe_ui`

这样外部 LLM 就能以“观察 -> 行动 -> 验证 -> 再观察”的方式做自主探索测试，而不需要在 Qt 应用内置任何模型。
