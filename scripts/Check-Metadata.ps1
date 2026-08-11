<#
.SYNOPSIS
    Asserts the version and the AppUserModelID agree everywhere they are
    declared, and optionally that a release tag agrees with the version.

.DESCRIPTION
    Both values are written by hand in files that cannot include each other:
    installer.toml is TOML, pingy.manifest is XML, pingy.rc is an RC script and
    crt_mini.cpp is C++. Nothing makes them agree, so this does, and CI runs it
    on every push.

    Both failures are silent. A drifted version means the Add/Remove Programs
    entry and the exe's Details tab disagree, and nobody notices until they are
    working out which build a machine is running. A drifted AUMID means the
    running window will not group under its own pinned taskbar icon.

.PARAMETER Tag
    A release tag such as v1.0.1. Compared against the version with any leading
    v removed. Omit it outside a release.
#>

[CmdletBinding()]
param(
    [string]$Tag
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot

function Get-Declared {
    param([string]$RelativePath, [string]$Pattern)

    $path = Join-Path $RepoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $path)) { throw "not found: $RelativePath" }

    $match = Select-String -Path $path -Pattern $Pattern | Select-Object -First 1
    if (-not $match) { throw "$RelativePath does not match /$Pattern/" }
    return $match.Matches.Groups[1].Value
}

# 1.0.1, 1.0.1.0 and "1,0,1,0" are the same version written three ways.
function ConvertTo-Quad {
    param([string]$Version)

    $parts = @($Version -split '[,.]' | ForEach-Object { $_.Trim() } |
               Where-Object { $_ -ne '' })
    while ($parts.Count -lt 4) { $parts += '0' }
    return ($parts[0..3] -join '.')
}

$declared = [ordered]@{
    'installer.toml  product.version' =
        Get-Declared 'installer.toml' '^\s*version\s*=\s*"([^"]+)"'
    'pingy.manifest  assemblyIdentity' =
        Get-Declared 'src\pingy.manifest' 'assemblyIdentity\s+version="([^"]+)"'
    # Keys differ by more than case: PowerShell hash literal keys are
    # case-insensitive, so FILEVERSION and FileVersion would collide.
    'pingy.rc        FILEVERSION (numeric)' =
        Get-Declared 'src\pingy.rc' '^\s*FILEVERSION\s+([\d,\s]+?)\s*$'
    'pingy.rc        PRODUCTVERSION (numeric)' =
        Get-Declared 'src\pingy.rc' '^\s*PRODUCTVERSION\s+([\d,\s]+?)\s*$'
    'pingy.rc        FileVersion (string)' =
        Get-Declared 'src\pingy.rc' 'VALUE\s+"FileVersion",\s*"([^"]+)"'
    'pingy.rc        ProductVersion (string)' =
        Get-Declared 'src\pingy.rc' 'VALUE\s+"ProductVersion",\s*"([^"]+)"'
}

if ($Tag) {
    $declared['git tag'] = $Tag -replace '^v', ''
}

foreach ($entry in $declared.GetEnumerator()) {
    '{0,-34} {1,-10} -> {2}' -f $entry.Key, $entry.Value, (ConvertTo-Quad $entry.Value)
}

$unique = @($declared.Values | ForEach-Object { ConvertTo-Quad $_ } | Sort-Object -Unique)
if ($unique.Count -ne 1) {
    throw "version declarations disagree: $($unique -join ', ')"
}

Write-Host "`nall version declarations agree: $($unique[0])" -ForegroundColor Green

# The AppUserModelID the installer stamps on the shortcut, and the one the
# process claims for itself at startup, have to be the same string or the
# window will not group under its own pinned icon.
$aumid = [ordered]@{
    'installer.toml  product.aumid' =
        Get-Declared 'installer.toml' '^\s*aumid\s*=\s*"([^"]+)"'
    'crt_mini.cpp    SetCurrentProcessExplicitAppUserModelID' =
        Get-Declared 'src\crt_mini.cpp' 'SetCurrentProcessExplicitAppUserModelID\(L"([^"]+)"\)'
}

''
foreach ($entry in $aumid.GetEnumerator()) {
    '{0,-56} {1}' -f $entry.Key, $entry.Value
}

$uniqueAumid = @($aumid.Values | Sort-Object -Unique)
if ($uniqueAumid.Count -ne 1) {
    throw "AppUserModelID declarations disagree: $($uniqueAumid -join ', ')"
}

Write-Host "`nAppUserModelID agrees: $($uniqueAumid[0])" -ForegroundColor Green
