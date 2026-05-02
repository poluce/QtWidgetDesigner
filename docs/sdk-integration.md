# QtAutoTest SDK 接入说明

`QtAutoTestRuntime` 现在已经被抽成一个可嵌入 Qt Widgets 应用的运行时 SDK。

它的职责是：

- 启动 Qt 自动化桥接服务
- 提供对象树 / 布局树 / 活动页面快照
- 提供查找、点击、输入、键盘、滚动、选择、等待、断言
- 给外部 MCP server 暴露稳定的底层能力

## SDK 结构

```text
sdk/
  include/qtautotest/
    action_observer.h
    automation_client.h
    automation_event_client.h
    build_harness.h
    install.h
    qtautotest.h
    harness.h
    runtime.h
    selector.h
    snapshot.h
    version.h
  src/
    ...

mcp/qtautotest-mcp/
  ...

examples/sdk-consumer/
  ...
```

## 接入方式

Qt 应用最小接入方式：

```cpp
#include <qtautotest/qtautotest.h>

QApplication app(argc, argv);

if (!qtautotest::install(app)) {
    qCritical() << qtautotest::installErrorString();
    return 2;
}
```

## 公开 API

当前对外公开的头文件包括：

- `qtautotest/qtautotest.h`
  聚合总头
- `qtautotest/runtime.h`
  Qt 应用内桥接运行时
- `qtautotest/install.h`
  推荐的极简安装入口
- `qtautotest/action_observer.h`
  动作观察扩展点，可由外部工程注入可视化或录制逻辑
- `qtautotest/automation_client.h`
  覆盖全部同步 bridge 命令的 typed 自动化客户端
- `qtautotest/automation_event_client.h`
  覆盖全部事件订阅能力的 typed 事件客户端
- `qtautotest/build_harness.h`
  configure / build / run / wait-ready 一体化 harness
- `qtautotest/harness.h`
  外部进程启动/停止/等待就绪
- `qtautotest/selector.h`
  结构化 selector 构造器
- `qtautotest/snapshot.h`
  快照、布局树、样式树、窗口截图等 typed 视图模型
- `qtautotest/version.h`
  版本宏

## 外部进程 Harness

如果你不是把 Runtime 嵌进当前进程，而是想启动另一个 Qt 应用再等待桥接就绪，可以用：

```cpp
#include <qtautotest/qtautotest.h>

qtautotest::ProcessHarness harness;
qtautotest::HarnessOptions options;
options.program = "MyQtApp.exe";
options.arguments = {"--port", "49555"};
options.bridgeUrl = QUrl("ws://127.0.0.1:49555");

harness.start(options);
harness.waitUntilReady();
```

这个能力很适合“LLM 先改代码，再启动应用，再自测”的闭环。

## 高层客户端

`AutomationClient` 现在就是公开层的主入口，全部 bridge 命令都已经有 typed API：

```cpp
qtautotest::AutomationClient client;
const auto clickResult = client.click(qtautotest::Selector::byObjectName("loginButton"));
if (!clickResult) {
    qCritical() << clickResult.error.message;
}
```

它优先覆盖最常见的观察、点击、输入、等待与 tab 切换场景。

事件订阅客户端也有对应的 typed 入口：

```cpp
qtautotest::AutomationEventClient events;
events.setTabChangedHandler([](const qtautotest::TabChangedEvent& event) {
    qDebug() << event.text;
});
events.connectToBridge();
events.subscribe({{qtautotest::EventKind::TabChanged}});
```

## BuildHarness

如果你想把“配置 -> 构建 -> 启动 -> wait-ready”串成一个统一入口，可以使用：

```cpp
qtautotest::BuildHarness harness;
qtautotest::BuildHarnessOptions options;
// configure / build / run options ...
harness.run(options);
harness.waitUntilReady();
```

## CMake 集成

安装后，外部项目可以这样写：

```cmake
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(QtAutoTest REQUIRED CONFIG)

add_executable(MyQtApp main.cpp)
target_link_libraries(MyQtApp PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    QtAutoTest::Runtime
)
```

## MCP 连接

接入 Runtime 的 Qt 应用启动后，可以继续使用：

```powershell
QtAgentMcpServer.exe --bridge-url ws://127.0.0.1:49555
```

这样外部 LLM 就不需要理解 Qt 细节，只要调用 MCP tools 即可。

仓库中的 demo app 现在是一个独立工程，不属于 SDK 主构建链。

## 脚本工具

仓库还提供了几份面向接入便利性的脚本：

- `tools/check-qt-environment.ps1`
- `tools/verify-runtime-bridge.ps1`
- `tools/scaffold-minimal-app.ps1`

## 当前状态

这版已经形成了基本 SDK 骨架，但还有几项值得后续补齐：

- 更丰富的 typed 高层 helper，例如菜单、Dock、拖拽、Splitter 等新增命令落地后的同步封装
- 在 `ProcessHarness` 之上继续增强 `BuildHarness`，补齐 smoke test / 自测回调 / 结果归档
- 菜单栏 / 右键菜单 / 工具栏的专门支持
- Dock、拖拽、Splitter、复杂滚动等更完整的桌面交互语义
- 更清晰的 public/private API 边界
- 更完整的安装后 consumer 验证
- 报告导出与录制回放层

更细的能力拆分与优先级见 [roadmap.md](roadmap.md)。

## 一键验证

仓库内置了 PowerShell 脚本：

```powershell
pwsh -NoLogo -Command ./tools/verify-sdk-install.ps1
```

它会自动执行：

1. 构建主工程
2. 安装 SDK 到 `build/install`
3. 使用安装后的 package config 配置 consumer 示例
4. 构建 consumer 示例
