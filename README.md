# QtAutoTest SDK

一个面向 Qt Widgets 应用的 LLM 工具接入 SDK。

这个仓库现在提供两层能力：

1. `QtAutoTestRuntime`
   一个可嵌入 Qt 应用的运行时桥接 SDK。
2. `QtAgentMcpServer`
   一个独立的 MCP stdio server，把运行时桥接映射成外部 LLM 可直接调用的 tools。
3. `QtAgentAutotest`
   一个中文 demo app，用来展示 SDK 能力。

它不内置 LLM 模型。真正的自主探索发生在外部 LLM 一侧，外部 Agent 通过 MCP tools 调用接入到目标 Qt 程序中的 SDK 能力。

## 已提供的原子能力

`QtAutoTestRuntime` 目前支持：

- 读取对象树、布局树与活动页面快照
- 枚举和聚焦顶层窗口
- 基于 `ref` / `path` / 祖先条件精确查找控件
- 模拟真实点击、文本输入、按键与快捷键
- 模拟滚动、切换标签页、切换堆叠页面、选择项、展开折叠树节点
- 断言控件状态
- 等待控件出现 / 状态满足
- 等待日志出现
- 读取 Qt 应用日志
- 截图当前窗口

接入了这个 SDK 的 Qt 应用，外部 LLM 可以基于这些原子能力完成：

- 自主探索测试
  先读 UI 树，再自己决定点哪里、测什么。
- 目标驱动测试
  接到“测试登录流程和计数器”后，自己拆解步骤、执行并验证。

## 当前可展示场景

当前 demo app 除了登录和计数器，还新增了几组更适合现场展示的场景：

- 异步用户加载
  点击 `loadUsersButton` 后，进度条推进，稍后用户列表和状态标签更新。
- 选项面板
  包含复选框、单选框和下拉框，适合演示 `toggle_check` 与 `choose_combo_option`。
- 多标签页与堆叠页面
  适合演示 `activate_tab`、`switch_stacked_page` 与当前页面上下文。
- 树形与表格
  适合演示 `expand_tree_node`、`select_item`。
- 滚动区域
  适合演示 `scroll` 与 `scroll_into_view`。
- 延迟揭示隐藏面板
  点击 `revealSecretButton` 后，隐藏区域会在稍后出现，适合演示等待类工具。
- 可交互确认对话框
  点击 `openConfirmDialogButton` 后会弹出一个真正的顶层 `QDialog`，外部 LLM 可以继续识别并点击确认/取消。
- 双日志面板
  左侧显示更适合人看的场景事件，右侧实时滚动显示桥接命令和应用日志。
- 可视化演示模式
  用户能看到光标慢速移动、高亮框、逐字输入和按钮按下释放过程。

## 核心架构

- `QtAutoTestRuntime`
  可嵌入 Qt 应用的运行时 SDK。
- `AgentBridgeServer`
  运行时 SDK 内的本地 WebSocket 服务，默认监听 `127.0.0.1:49555`。
- `WidgetIntrospection`
  负责输出对象树、布局树、活动页面快照和精确查找。
- `UiActionExecutor`
  负责用 `QTest` 模拟点击、输入、键盘、滚动和选择类操作。
- `BridgeOperations`
  负责断言、等待和树模型桥接命令。
- `AppLogSink`
  捕获 `qDebug/qInfo/qWarning/qCritical` 日志，供外部代理读取。
- `QtAgentMcpServer`
  独立 MCP server，可被外部 LLM/Agent 当成工具服务器使用。
- `QtAgentAutotest`
  中文示例应用，只是 SDK 的演示承载体，不是 SDK 本体。

## Bridge 命令

- `ping`
- `describe_ui`
- `describe_snapshot`
- `describe_object_tree`
- `describe_layout_tree`
- `describe_subtree`
- `describe_active_page`
- `list_windows`
- `focus_window`
- `find_widgets`
- `click`
- `set_text`
- `press_key`
- `send_shortcut`
- `scroll`
- `scroll_into_view`
- `select_item`
- `toggle_check`
- `choose_combo_option`
- `activate_tab`
- `switch_stacked_page`
- `expand_tree_node`
- `collapse_tree_node`
- `assert_widget`
- `wait_for_widget`
- `wait_for_log`
- `get_logs`
- `capture_window`
- `list_commands`

详细协议见 [docs/agent-protocol.md](docs/agent-protocol.md)。
MCP 适配说明见 [docs/mcp-tool-server.md](docs/mcp-tool-server.md)。

## 构建

### Qt 5.14.2 示例

```powershell
cmake -S . -B build `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH=E:/Qt/Qt5.14.2/5.14.2/mingw73_64 `
  -DCMAKE_MAKE_PROGRAM=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/mingw32-make.exe `
  -DCMAKE_CXX_COMPILER=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/g++.exe

cmake --build build -j 4
```

这会生成：

- `build/QtAgentAutotest.exe`
- `build/QtAgentMcpServer.exe`
- `build/libQtAutoTestRuntime.a`

## 运行 Demo App

```powershell
./build/QtAgentAutotest.exe --port 49555
```

如果要给用户做“看得见动作过程”的现场演示，可以打开可视化演示模式：

```powershell
./build/QtAgentAutotest.exe --port 49555 --demo-visible
```

也可以进一步控制演示倍速：

```powershell
./build/QtAgentAutotest.exe --port 49555 --demo-visible --demo-speed 0.5
./build/QtAgentAutotest.exe --port 49555 --demo-visible --demo-speed 1
./build/QtAgentAutotest.exe --port 49555 --demo-visible --demo-speed 2
```

开启后，用户可以在屏幕上看到：

- 应用内模拟光标慢速移动到目标控件附近，不会抢走用户真实鼠标
- 当前即将操作的控件被高亮
- 输入框里逐字出现文字
- 按钮按下和释放的过程，以及点击脉冲效果
- 所有这些演示节奏会跟随 `--demo-speed` 一起变快或变慢

## 运行 MCP server

```powershell
./build/QtAgentMcpServer.exe --bridge-url ws://127.0.0.1:49555
```

## 外部 LLM 接入方式

外部支持 MCP 的 LLM/Agent 只需要连接 `QtAgentMcpServer`。

一个最小配置示意：

```json
{
  "mcpServers": {
    "qt-agent": {
      "command": "F:/B_My_Document/GitHub/QtWidgetDesigner/build/QtAgentMcpServer.exe",
      "args": [
        "--bridge-url",
        "ws://127.0.0.1:49555"
      ]
    }
  }
}
```

## 设计取舍

- 当前日志指的是 Qt 应用日志，不是 Windows 事件日志。
- 当前自动化目标是 Qt Widgets，不包含 QML 和嵌入式浏览器 DOM。
- 当前 MCP server 只负责工具适配，不做测试规划，不内置模型。
- 当前桥接层默认只暴露低风险原子动作，避免外部探索时做 destructive 操作。

## SDK 验证

仓库内置了一个 SDK 安装与 consumer 验证脚本：

```powershell
pwsh -NoLogo -Command ./tools/verify-sdk-install.ps1
```

它会自动完成：

1. 构建主工程
2. 安装 `QtAutoTestRuntime`
3. 配置 `examples/sdk-consumer`
4. 构建 consumer 示例

详细接入说明见 [docs/sdk-integration.md](docs/sdk-integration.md)。
