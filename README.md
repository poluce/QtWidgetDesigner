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

## 快速开始

对 Qt Widgets 应用的最小接入现在可以压到 1-3 行：

```cpp
#include <qtautotest/qtautotest.h>

QApplication app(argc, argv);
if (!qtautotest::install(app)) {
    qCritical() << qtautotest::installErrorString();
    return 2;
}
```

安装后，外部就可以通过 `QtAgentMcpServer` 或 SDK 客户端与应用通信。

接入了这个 SDK 的 Qt 应用，外部 LLM 可以基于这些原子能力完成：

- 自主探索测试
  先读 UI 树，再自己决定点哪里、测什么。
- 目标驱动测试
  接到“测试登录流程和计数器”后，自己拆解步骤、执行并验证。

## 使用方式

最常见的使用路径可以分成 4 类：

1. 把 Runtime 嵌进你的 Qt Widgets 应用
2. 用 `AutomationClient` 直接调用 typed SDK
3. 用 `AutomationEventClient` 订阅 UI 事件
4. 用 `ProcessHarness` / `BuildHarness` 跑“启动或构建后自测”

### 1. 嵌入到应用

如果你要让自己的 Qt Widgets 程序暴露自动化 bridge，通常只需要改启动入口：

```cpp
#include <qtautotest/qtautotest.h>

QApplication app(argc, argv);

qtautotest::InstallOptions options;
options.port = 49555;

if (!qtautotest::install(app, options)) {
    qCritical() << qtautotest::installErrorString();
    return 2;
}
```

这一步做完后，应用进程内会启动本地 bridge，默认监听 `ws://127.0.0.1:49555`。

### 2. 直接调用 typed SDK

如果你想从另一个进程直接控制 bridge，主入口是 `AutomationClient`：

```cpp
#include <qtautotest/qtautotest.h>

qtautotest::AutomationClient client(QUrl("ws://127.0.0.1:49555"));

const auto snapshot = client.describeSnapshot();
if (!snapshot) {
    qCritical() << snapshot.error.message;
    return;
}

const auto click = client.click(qtautotest::Selector::byObjectName("loginButton"));
if (!click) {
    qCritical() << click.error.message;
    return;
}
```

几个常用调用长这样：

```cpp
qtautotest::AutomationClient client;

client.setText(qtautotest::Selector::byObjectName("usernameEdit"), "alice");
client.setText(qtautotest::Selector::byObjectName("passwordEdit"), "123456");
client.click(qtautotest::Selector::byObjectName("loginButton"));

qtautotest::WidgetAssertions assertions;
assertions.expectTextContains("欢迎");

const auto waitResult = client.waitForWidget(
    qtautotest::Selector::byObjectName("statusLabel"),
    assertions);
```

如果你要做只读探索，通常先从这些方法开始：

- `describeSnapshot()`
- `describeObjectTree(...)`
- `describeLayoutTree(...)`
- `describeStyle(...)`
- `findWidgets(...)`
- `listWindows()`

如果你要做动作和验证，通常会用：

- `click(...)`
- `setText(...)`
- `pressKey(...)`
- `sendShortcut(...)`
- `scroll(...)`
- `selectListItem(...)` / `selectTreeItem(...)` / `selectTableCell(...)`
- `chooseComboOption(...)`
- `activateTab(...)`
- `assertWidget(...)`
- `waitForWidget(...)`
- `waitForLog(...)`

### 3. 订阅 typed 事件

如果你不想轮询，而是想在 tab 切换、焦点变化、模态对话框弹出时收到事件，可以用 `AutomationEventClient`：

```cpp
#include <qtautotest/qtautotest.h>

qtautotest::AutomationEventClient events(QUrl("ws://127.0.0.1:49555"));

events.setTabChangedHandler([](const qtautotest::TabChangedEvent& event) {
    qDebug() << "tab changed:" << event.text << event.index;
});

events.setFocusWidgetChangedHandler([](const qtautotest::FocusWidgetChangedEvent& event) {
    qDebug() << "focus widget:" << event.widget.objectName;
});

if (!events.connectToBridge()) {
    qCritical() << events.errorString();
    return;
}

qtautotest::EventSubscriptionSpec spec;
spec.events = {
    qtautotest::EventKind::TabChanged,
    qtautotest::EventKind::FocusWidgetChanged,
};
spec.selectors = {
    qtautotest::Selector::byObjectName("demoTabWidget"),
};

const auto subscription = events.subscribe(spec);
if (!subscription) {
    qCritical() << subscription.error.message;
}
```

当前已经 typed 化的事件有：

- `TabChangedEvent`
- `ActivePageChangedEvent`
- `WindowFocusChangedEvent`
- `FocusWidgetChangedEvent`
- `ModalDialogChangedEvent`

### 4. 启动后再测

如果目标程序不是当前进程，而是“先启动一个 Qt 应用，再等 bridge ready，再执行动作”，可以用 `ProcessHarness`：

```cpp
#include <qtautotest/qtautotest.h>

qtautotest::ProcessHarness harness;
qtautotest::HarnessOptions options;
options.program = "MyQtApp.exe";
options.arguments = {"--port", "49555"};
options.bridgeUrl = QUrl("ws://127.0.0.1:49555");

if (!harness.start(options) || !harness.waitUntilReady()) {
    qCritical() << harness.errorString();
    return;
}

auto client = harness.automationClient();
client.ping();
```

如果你要把“配置 -> 构建 -> 启动 -> 等待 ready”串起来，可以用 `BuildHarness`：

```cpp
#include <qtautotest/qtautotest.h>

qtautotest::BuildHarness harness;
qtautotest::BuildHarnessOptions options;

options.configure.program = "cmake";
options.configure.arguments = {"-S", ".", "-B", "build"};

options.build.program = "cmake";
options.build.arguments = {"--build", "build", "-j", "4"};

options.run.program = "build/MyQtApp.exe";
options.run.bridgeUrl = QUrl("ws://127.0.0.1:49555");

if (!harness.run(options) || !harness.waitUntilReady()) {
    qCritical() << harness.errorString();
    return;
}

auto client = harness.automationClient();
client.describeSnapshot();
```

### 5. 通过 MCP 给外部 LLM 用

如果你想让外部支持 MCP 的 Agent 直接把 Qt 应用当工具来调用，流程是：

1. 先把 `QtAutoTestRuntime` 嵌进你的 Qt 应用
2. 启动应用，让 bridge 监听本地端口
3. 再启动 `QtAgentMcpServer`

```powershell
./build/QtAgentMcpServer.exe --bridge-url ws://127.0.0.1:49555
```

然后在外部 LLM/Agent 的 MCP 配置里指向这个可执行文件即可。

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

## 公开 SDK 入口

- `install(...)`
  推荐的极简接入入口
- `AutomationClient`
  更高层、typed 的常用自动化客户端
- `AutomationEventClient`
  强类型事件订阅客户端
- `BuildHarness`
  统一封装 configure / build / run / wait-ready
- `ProcessHarness`
  进程启动与 bridge ready 等待

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

如果你的机器上同时存在 `mingw73_32` 和 `mingw73_64`，建议把 64 位 Qt 和 64 位 MinGW 的 `bin` 路径放到 `PATH` 更前面，避免误混进 32 位运行库。

这会生成：

- `build/QtAgentMcpServer.exe`
- `build/libQtAutoTestRuntime.a`

## 运行 MCP server

```powershell
./build/QtAgentMcpServer.exe --bridge-url ws://127.0.0.1:49555
```

如果你要运行仓库内的独立 demo，请看 [examples/demo-app/README.md](examples/demo-app/README.md)。

如果你要从模板生成一个最小接入工程，可以使用：

```powershell
pwsh -NoLogo -Command ./tools/scaffold-minimal-app.ps1 -TargetDir C:/temp/MyQtAutoTestApp -ProjectName MyQtAutoTestApp
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

## 当前缺口

当前最小闭环已经可用，但离完整桌面自动化 SDK 还有一些明确缺口：

- 菜单栏 / 右键菜单 / 工具栏的专门支持
- Dock 窗口更细粒度操作
- 拖拽与 Splitter 拖动
- 更强的 scroll 语义，尤其复杂视图内部滚动
- 更完整的 `BuildHarness`，补齐 smoke test / 自测回调 / 结果归档

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

## 便利性工具

- `tools/check-qt-environment.ps1`
  检查 Qt / MinGW / CMake 基础环境
- `tools/verify-runtime-bridge.ps1`
  启动一个 Qt 程序并验证 bridge `ping`
- `tools/scaffold-minimal-app.ps1`
  生成最小 QtAutoTest 接入模板
