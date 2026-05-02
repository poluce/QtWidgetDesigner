param(
    [Parameter(Mandatory = $true)]
    [string]$TargetDir,
    [string]$ProjectName = "QtAutoTestMinimalApp"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$templateDir = Join-Path $repoRoot "examples/minimal-app-template"

if (-not (Test-Path $templateDir)) {
    throw "Template directory not found: $templateDir"
}

if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir | Out-Null
}

Copy-Item -Path (Join-Path $templateDir "*") -Destination $TargetDir -Recurse -Force

$files = Get-ChildItem -Path $TargetDir -File -Recurse
foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw
    $content = $content.Replace("__PROJECT_NAME__", $ProjectName)
    Set-Content -Path $file.FullName -Value $content -NoNewline
}

Write-Host "Scaffolded minimal app at $TargetDir" -ForegroundColor Green
