[CmdletBinding()]
param(
    [string]$InstallerPath = (Join-Path $PSScriptRoot '..\dist\TraceGraph-Studio-Setup-0.1.0.exe')
)

$ErrorActionPreference = 'Stop'
$resolvedInstaller = [System.IO.Path]::GetFullPath($InstallerPath)

if (-not (Test-Path -LiteralPath $resolvedInstaller -PathType Leaf)) {
    throw "Installer does not exist: $resolvedInstaller"
}

$installer = Get-Item -LiteralPath $resolvedInstaller
if ($installer.Length -lt 1MB) {
    throw "Installer is unexpectedly small ($($installer.Length) bytes): $resolvedInstaller"
}

$signature = Get-AuthenticodeSignature -LiteralPath $resolvedInstaller
if ($signature.Status -notin @('Valid', 'NotSigned')) {
    throw "Installer has an invalid Authenticode status: $($signature.Status)"
}

Write-Host "Installer verified: $resolvedInstaller ($($installer.Length) bytes, signature: $($signature.Status))"
