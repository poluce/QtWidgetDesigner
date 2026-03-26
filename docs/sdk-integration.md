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
    qtautotest.h
    actions.h
    bridge_client.h
    harness.h
    runtime.h
    selector.h
    snapshot.h
    version.h
  src/
    ...

mcp/qtautotest-mcp/
  ...

examples/demo-app/
  ...

examples/sdk-consumer/
  ...
```

## 接入方式

Qt 应用最小接入方式：

```cpp
#include <qtautotest/qtautotest.h>

qtautotest::Runtime runtime;
qtautotest::RuntimeOptions options;
options.port = 49555;
options.enableVisibleDemo = false;
options.demoSpeed = 1.0;

runtime.start(options);
```

## 公开 API

当前对外公开的头文件包括：

- `qtautotest/qtautotest.h`
  聚合总头
- `qtautotest/runtime.h`
  Qt 应用内桥接运行时
- `qtautotest/bridge_client.h`
  直接调用桥接层的客户端
- `qtautotest/harness.h`
  外部进程启动/停止/等待就绪
- `qtautotest/selector.h`
  结构化 selector 构造器
- `qtautotest/actions.h`
  常见动作参数构造器
- `qtautotest/snapshot.h`
  快照与节点轻量视图封装
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

## 当前状态

这版已经形成了基本 SDK 骨架，但还有几项值得后续补齐：

- 更 typed 的 SDK API，逐步减少 action / result 直接暴露 `QJsonObject`
- 在 `ProcessHarness` 之上补齐“构建 -> 启动 -> 自测”的 `BuildHarness`
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
