[CmdletBinding()]
param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '..\build'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\dist'),
    [string]$QtDirectory,
    [string]$InnoCompiler
)

$ErrorActionPreference = 'Stop'
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = [System.IO.Path]::GetFullPath($BuildDirectory)
$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
$stagingRoot = Join-Path $buildRoot 'package\TraceGraph Studio'
$appExe = Join-Path $buildRoot 'bin\tracegraph-studio.exe'
$cachePath = Join-Path $buildRoot 'CMakeCache.txt'

if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
    throw "CMake cache not found: $cachePath"
}

& cmake --build $buildRoot
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed with exit code $LASTEXITCODE"
}
if (-not (Test-Path -LiteralPath $appExe -PathType Leaf)) {
    throw "Built executable not found: $appExe"
}

if (-not $QtDirectory) {
    $qtCacheLine = Select-String -LiteralPath $cachePath -Pattern '^CMAKE_PREFIX_PATH:[^=]*=(.+)$' | Select-Object -First 1
    if ($qtCacheLine) {
        $QtDirectory = $qtCacheLine.Matches[0].Groups[1].Value
    }
}
if (-not $QtDirectory) {
    throw 'QtDirectory was not supplied and CMAKE_PREFIX_PATH is absent from CMakeCache.txt'
}

$deployTool = Join-Path $QtDirectory 'bin\windeployqt.exe'
if (-not (Test-Path -LiteralPath $deployTool -PathType Leaf)) {
    throw "windeployqt was not found: $deployTool"
}

if (-not $InnoCompiler) {
    $isccCommand = Get-Command iscc.exe -ErrorAction SilentlyContinue
    if ($isccCommand) {
        $InnoCompiler = $isccCommand.Source
    } else {
        $candidateCompilers = @(
            (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
            (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe')
        )
        $InnoCompiler = $candidateCompilers | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
    }
}
if (-not $InnoCompiler -or -not (Test-Path -LiteralPath $InnoCompiler -PathType Leaf)) {
    throw 'Inno Setup Compiler (ISCC.exe) was not found. Install JRSoftware.InnoSetup or pass -InnoCompiler.'
}

if (Test-Path -LiteralPath $stagingRoot) {
    $resolvedBuildRoot = [System.IO.Path]::GetFullPath($buildRoot).TrimEnd('\') + '\'
    $resolvedStagingRoot = [System.IO.Path]::GetFullPath($stagingRoot)
    if (-not $resolvedStagingRoot.StartsWith($resolvedBuildRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean staging directory outside build root: $resolvedStagingRoot"
    }
    Remove-Item -LiteralPath $resolvedStagingRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $stagingRoot -Force | Out-Null
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
Copy-Item -LiteralPath $appExe -Destination $stagingRoot
Copy-Item -LiteralPath (Join-Path $repoRoot 'samples') -Destination $stagingRoot -Recurse
Copy-Item -LiteralPath (Join-Path $repoRoot 'README.md') -Destination $stagingRoot

& $deployTool --release --compiler-runtime --force --no-translations (Join-Path $stagingRoot 'tracegraph-studio.exe')
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

$cmakeProject = Get-Content -LiteralPath (Join-Path $repoRoot 'CMakeLists.txt') -Raw
$versionMatch = [regex]::Match($cmakeProject, 'project\([^\r\n]*\bVERSION\s+([0-9]+(?:\.[0-9]+){1,3})')
if (-not $versionMatch.Success) {
    throw 'Could not determine application version from CMakeLists.txt'
}
$appVersion = $versionMatch.Groups[1].Value

$sourceDefine = "/DAppSourceDir=$stagingRoot"
$outputDefine = "/DAppOutputDir=$outputRoot"
$versionDefine = "/DAppVersion=$appVersion"
& $InnoCompiler $sourceDefine $outputDefine $versionDefine (Join-Path $PSScriptRoot 'tracegraph-studio.iss')
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup compilation failed with exit code $LASTEXITCODE"
}

$installerPath = Join-Path $outputRoot "TraceGraph-Studio-Setup-$appVersion.exe"
if (-not (Test-Path -LiteralPath $installerPath -PathType Leaf)) {
    throw "Expected installer was not created: $installerPath"
}

Write-Host "Installer created: $installerPath"
