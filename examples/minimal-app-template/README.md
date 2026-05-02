# __PROJECT_NAME__

这是一个最小 QtAutoTest 接入模板。

接入代码只有一处：

```cpp
if (!qtautotest::install(app)) {
    qCritical() << qtautotest::installErrorString();
    return 2;
}
```

配置方式：

```powershell
cmake -S . -B build `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="F:/path/to/QtAutoTest/install;E:/Qt/Qt5.14.2/5.14.2/mingw73_64" `
  -DCMAKE_MAKE_PROGRAM=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/mingw32-make.exe `
  -DCMAKE_CXX_COMPILER=E:/Qt/Qt5.14.2/Tools/mingw730_64/bin/g++.exe

cmake --build build -j 4
```
