# QtAutoTest Demo

这是一个独立于 SDK 主工程的演示应用工程，用来展示 `QtAutoTestRuntime` + `QtAgentMcpServer`
在真实 Qt Widgets 界面上的交互效果。

它不是 SDK 本体，也不参与 SDK 安装包分发。

## 构建前提

先准备好可用的 Qt 和一个可发现的 `QtAutoTest` package，推荐两种方式：

1. 使用根工程的 build tree
2. 先安装 SDK，再使用安装目录

## 使用根工程 build tree 构建

先在仓库根目录构建 SDK/MCP：

```powershell
cmake -S . -B build `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH=E:/Qt/Qt5.14.2/5.14.2/mingw73_64 `
  -DCMAKE_MAKE_PROGRAM=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/mingw32-make.exe `
  -DCMAKE_CXX_COMPILER=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/g++.exe

cmake --build build -j 4
```

再单独配置 demo：

```powershell
cmake -S examples/demo-app -B build/demo-app `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="F:/B_My_Document/GitHub/QtWidgetDesigner/build;E:/Qt/Qt5.14.2/5.14.2/mingw73_64" `
  -DCMAKE_MAKE_PROGRAM=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/mingw32-make.exe `
  -DCMAKE_CXX_COMPILER=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/g++.exe

cmake --build build/demo-app -j 4
```

## 运行 Demo

```powershell
./build/demo-app/QtAutoTestDemo.exe --port 49555
```

如果要做“看得见动作过程”的现场演示：

```powershell
./build/demo-app/QtAutoTestDemo.exe --port 49555 --demo-visible
./build/demo-app/QtAutoTestDemo.exe --port 49555 --demo-visible --demo-speed 0.5
./build/demo-app/QtAutoTestDemo.exe --port 49555 --demo-visible --demo-speed 1
./build/demo-app/QtAutoTestDemo.exe --port 49555 --demo-visible --demo-speed 2
```

## 启动 MCP Server

SDK/MCP 仍由根工程产出：

```powershell
./build/QtAgentMcpServer.exe --bridge-url ws://127.0.0.1:49555
```

## 演示场景

- 登录表单与计数器
- 选项面板
- 异步用户加载
- 树形与表格
- 多标签页与堆叠页面
- 滚动区域
- 可交互确认对话框
- 双日志面板
- 可视化演示模式
