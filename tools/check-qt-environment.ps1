param(
    [string]$QtPrefix = "E:/Qt/Qt5.14.2/5.14.2/mingw73_64",
    [string]$MinGwBin = "E:/Qt/Qt5.14.2/Tools/mingw730_64/bin"
)

$ErrorActionPreference = "Stop"

function Test-Tool {
    param([string]$Path)
    [pscustomobject]@{
        Path = $Path
        Exists = (Test-Path $Path)
    }
}

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
$qmakePath = Join-Path $QtPrefix "bin/qmake.exe"
$makePath = Join-Path $MinGwBin "mingw32-make.exe"
$gxxPath = Join-Path $MinGwBin "g++.exe"

[pscustomobject]@{
    CMakeAvailable = ($null -ne $cmake)
    CMakePath = if ($cmake) { $cmake.Source } else { "" }
    QtPrefixExists = (Test-Path $QtPrefix)
    MinGwBinExists = (Test-Path $MinGwBin)
    QMake = Test-Tool $qmakePath
    Make = Test-Tool $makePath
    Gxx = Test-Tool $gxxPath
} | Format-List
