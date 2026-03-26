param(
    [string]$QtPrefix = "E:/Qt/Qt5.14.2/5.14.2/mingw73_64",
    [string]$MinGwBin = "E:/Qt/Qt5.14.2/Tools/mingw730_64/bin",
    [string]$BuildDir = "build",
    [string]$InstallSubdir = "install",
    [string]$ConsumerSubdir = "consumer-build"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$env:PATH = "$MinGwBin;$QtPrefix/bin;$env:PATH"

$buildPath = Join-Path $repoRoot $BuildDir
$installPath = Join-Path $buildPath $InstallSubdir
$consumerBuildPath = Join-Path $buildPath $ConsumerSubdir

Write-Host "== QtAutoTest SDK 验证 ==" -ForegroundColor Cyan
Write-Host "仓库目录: $repoRoot"
Write-Host "Qt 前缀: $QtPrefix"
Write-Host "MinGW Bin: $MinGwBin"

Write-Host "`n[1/4] 构建主工程..." -ForegroundColor Yellow
cmake --build $buildPath -j 4
if ($LASTEXITCODE -ne 0) {
    throw "主工程构建失败"
}

Write-Host "`n[2/4] 安装 SDK..." -ForegroundColor Yellow
if (Test-Path $installPath) {
    Remove-Item -Recurse -Force $installPath
}
cmake --install $buildPath --prefix $installPath
if ($LASTEXITCODE -ne 0) {
    throw "SDK 安装失败"
}

Write-Host "`n[3/4] 配置 consumer 示例..." -ForegroundColor Yellow
if (Test-Path $consumerBuildPath) {
    Remove-Item -Recurse -Force $consumerBuildPath
}
$prefixPath = "$installPath;$QtPrefix"
$consumerSource = Join-Path $repoRoot "examples/sdk-consumer"
$makeProgram = Join-Path $MinGwBin "mingw32-make.exe"
$cxxCompiler = Join-Path $MinGwBin "g++.exe"
cmake -S $consumerSource `
      -B $consumerBuildPath `
      -G "MinGW Makefiles" `
      "-DCMAKE_PREFIX_PATH=$prefixPath" `
      "-DCMAKE_MAKE_PROGRAM=$makeProgram" `
      "-DCMAKE_CXX_COMPILER=$cxxCompiler"
if ($LASTEXITCODE -ne 0) {
    throw "consumer 示例配置失败"
}

Write-Host "`n[4/4] 构建 consumer 示例..." -ForegroundColor Yellow
cmake --build $consumerBuildPath -j 4
if ($LASTEXITCODE -ne 0) {
    throw "consumer 示例构建失败"
}

Write-Host "`n验证通过。" -ForegroundColor Green
Write-Host "安装目录: $installPath"
Write-Host "Consumer 构建目录: $consumerBuildPath"
