# QtAutoTest SDK

一个面向 Qt Widgets 应用的 LLM 工具接入 SDK。

这个仓库现在提供两层能力：

1. `QtAutoTestRuntime`
   一个可嵌入 Qt 应用的运行时桥接 SDK。
2. `QtAgentMcpServer`
   一个独立的 MCP stdio server，把运行时桥接映射成外部 LLM 可直接调用的 tools。

它不内置 LLM 模型。真正的自主探索发生在外部 LLM 一侧，外部 Agent 通过 MCP tools 调用接入到目标 Qt 程序中的 SDK 能力。

仓库内另有一个独立 demo 工程，见 [examples/demo-app/README.md](examples/demo-app/README.md)。

## 已提供的原子能力

`QtAutoTestRuntime` 目前支持：

- 读取对象树、布局树与活动页面快照
- 读取标准控件的 palette 颜色、styleSheet 与样式摘要
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

## Bridge 命令

- `ping`
- `describe_ui`
- `describe_snapshot`
- `describe_object_tree`
- `describe_layout_tree`
- `describe_subtree`
- `describe_style`
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

- `build/QtAgentMcpServer.exe`
- `build/libQtAutoTestRuntime.a`

## 运行 MCP server

```powershell
./build/QtAgentMcpServer.exe --bridge-url ws://127.0.0.1:49555
```

如果你要运行仓库内的独立 demo，请看 [examples/demo-app/README.md](examples/demo-app/README.md)。

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

## 当前缺口

当前最小闭环已经可用，但离完整桌面自动化 SDK 还有一些明确缺口：

- 菜单栏 / 右键菜单 / 工具栏的专门支持
- Dock 窗口更细粒度操作
- 拖拽与 Splitter 拖动
- 更强的 scroll 语义，尤其复杂视图内部滚动
- 更 typed 的 SDK API，减少直接暴露 `QJsonObject`
- 更完整的 `BuildHarness`，补齐“构建 -> 启动 -> 自测”

更详细的现状与优先级说明见 [docs/roadmap.md](docs/roadmap.md)。

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
