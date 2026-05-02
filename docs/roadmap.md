# QtAutoTest SDK Roadmap

这份路线图用于描述当前 SDK / MCP 能力边界，以及下一阶段需要补齐的关键缺口。

## 当前基线

仓库当前已经具备一条可工作的闭环：

- Qt Widgets 应用内嵌 `QtAutoTestRuntime`
- 运行时通过本地 bridge 暴露结构化观察与原子动作
- `QtAgentMcpServer` 把 bridge 命令映射成 MCP tools
- `ProcessHarness` 可以完成“启动目标程序 -> 等待桥接 ready -> 执行调用”

当前已覆盖的动作和复杂控件主要包括：

- 点击、输入、按键、快捷键
- 基础滚动与 `scroll_into_view`
- 列表、树、表格选择
- 标签页、堆叠页切换
- 复选框 / 单选框 / 下拉框
- 树节点展开折叠
- 窗口聚焦、日志读取、等待、断言、截图

这意味着“观察 -> 行动 -> 验证”的最小自动化链路已经成立，但复杂交互面、SDK 易用性和工程化闭环仍然不够完整。

## 交互能力缺口

### 菜单栏 / 右键菜单 / 工具栏

当前能力更偏向普通 widget 直接交互，还缺少对以下入口的专门支持：

- 菜单栏动作枚举与触发
- 右键菜单弹出后的菜单项识别与点击
- 工具栏动作、分隔按钮、溢出区域的识别与触发

这类能力补齐后，Agent 才能更稳定地覆盖典型桌面应用常见入口，而不必退化成坐标级猜测。

### Dock 窗口更细粒度操作

当前可以识别和聚焦顶层窗口，但对 `QDockWidget` 这类桌面工作区结构还缺少专门语义，例如：

- 枚举 dock、识别停靠区域与 tabified 关系
- show / hide / raise / activate
- float / dock 回去
- 切换 dock tab
- 面向 dock 标题栏、关闭按钮、停靠状态的断言

### 拖拽

当前动作集合以 click / key / select 为主，还没有系统性的 drag-and-drop 抽象，包括：

- 按 widget 到 widget 的拖拽
- 按模型项到模型项的拖拽
- 带路径、索引或目标区域的拖放定位
- 拖拽开始、悬停、释放后的结果验证

### Splitter 拖动

`QSplitter` 是桌面 UI 中常见布局控制点，但现在没有“按 handle 拖动”的专门动作，导致：

- 无法稳定调整主从视图比例
- 无法验证最小尺寸、折叠阈值与布局回流
- 复杂工作区场景难以自动复现

### 更强的 scroll 语义

当前 `scroll` / `scroll_into_view` 已能覆盖基础 `QScrollArea` 场景，但复杂视图内部滚动仍然偏弱，后续需要补齐：

- 嵌套滚动容器的目标选择
- 面向 viewport、item view、文本视图的统一滚动抽象
- 按方向、轴、绝对位置、相对增量滚动
- “滚到某一行/某个 item/某个 ref 可见”的更强语义
- 对复杂视图内部滚动后的可见性和位置断言

## SDK API 状态

### 全量 typed bridge SDK 已完成

当前公开 SDK 已经完成这几件事：

- 全部同步 bridge 命令都有 typed `AutomationClient` 方法
- 事件订阅有独立的 typed `AutomationEventClient`
- `Selector`、快照、布局树、样式树、等待结果、错误上下文都不再暴露 `QJsonObject`
- 原始 JSON bridge client 已经下沉到 SDK 内部编解码层

这意味着 consumer 项目现在可以在不接触协议 JSON 形状的前提下完成“观察 -> 行动 -> 验证 -> 订阅事件”的完整闭环。

### 更完整的 BuildHarness

当前已经有 `ProcessHarness` 和 `BuildHarness`，可以完成“配置 -> 构建 -> 启动 -> wait-ready”的闭环；但“启动后自动 smoke/self-test、结果归档、失败摘要”还不够完整。

后续建议继续增强 `BuildHarness`，统一补齐：

- 配置命令、构建命令、启动命令
- 工作目录、环境变量、产物路径
- 失败日志归集
- 构建成功后自动启动目标程序
- 可选的 smoke test / self-test 回调

这会让“LLM 改代码后自动构建并回归”成为 SDK 的一等能力，而不是每个 consumer 自己拼脚本。

## 推荐的实现顺序

如果按“最先提升 Agent 实用性”的角度排序，建议优先级如下：

1. 更强的 scroll 语义
2. 菜单栏 / 右键菜单 / 工具栏支持
3. Dock 窗口更细粒度操作
4. 拖拽与 Splitter 拖动
5. 更完整的 BuildHarness

前四项直接影响桌面 UI 的可操作面；最后一项更多影响工程化闭环。

## 与现有文档的关系

- 协议层现状见 `docs/agent-protocol.md`
- MCP tools 映射见 `docs/mcp-tool-server.md`
- SDK 接入方式见 `docs/sdk-integration.md`

这份文档只负责说明“当前还缺什么，以及为什么缺”。
