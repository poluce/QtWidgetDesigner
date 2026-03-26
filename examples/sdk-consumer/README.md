# QtAutoTestConsumer

这是一个最小外部使用方示例，用来验证：

- `QtAutoTestRuntime` 能被安装
- `find_package(QtAutoTest REQUIRED CONFIG)` 能找到 SDK
- 外部 Qt Widgets 应用可以只通过 `qtautotest::Runtime` 接入自动化桥接

## 关键点

- 不依赖仓库内部 `sdk/src` 私有头文件
- 只使用安装后的公开头文件：
  - `<qtautotest/qtautotest.h>`
  - `<qtautotest/runtime.h>`
- 只链接公开 target：
  - `QtAutoTest::Runtime`

## 典型验证流程

1. 先安装 SDK：

```powershell
cmake --install build --prefix build/install
```

2. 再配置并构建 consumer：

```powershell
cmake -S examples/sdk-consumer -B build/consumer-build `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="F:/B_My_Document/GitHub/QtWidgetDesigner/build/install;E:/Qt/Qt5.14.2/5.14.2/mingw73_64" `
  -DCMAKE_MAKE_PROGRAM=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/mingw32-make.exe `
  -DCMAKE_CXX_COMPILER=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/g++.exe

cmake --build build/consumer-build -j 4
```

## 运行结果

运行后会弹出一个最小 Qt 窗口，并在后台启动自动化桥接服务。
