[CmdletBinding()]
param(
  [Parameter(Position = 0)]
  [ValidateSet("install", "test", "uninstall", "update")]
  [string]$Mode = "install"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$AppName = "lowhunt"
$ThemeTag = "LowHunt"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$InstallRoot = Join-Path $env:LOCALAPPDATA "Programs\$AppName"
$UserBinDir = Join-Path $HOME ".local\bin"
$SystemLauncher = Join-Path $UserBinDir "$AppName.cmd"
$TestBase = Join-Path $ScriptDir "test-root"
$TestRoot = Join-Path $TestBase $AppName
$TestBinDir = Join-Path $TestBase "bin"
$TestLauncher = Join-Path $TestBinDir "$AppName.cmd"
$ManifestName = ".lowhunt-install.json"
$RuntimePayload = @("src", "data", "platforms", "tests", "man", "README.md", "LICENSE", "Makefile", "CMakeLists.txt")

function Write-Labelled {
  param([string]$Label, [string]$Message, [ConsoleColor]$Color = [ConsoleColor]::Gray)
  Write-Host ("[{0}][{1}] {2}" -f $ThemeTag, $Label, $Message) -ForegroundColor $Color
}

function Write-Info { param([string]$Message) Write-Labelled -Label "INFO" -Message $Message -Color Cyan }
function Write-Success { param([string]$Message) Write-Labelled -Label " OK " -Message $Message -Color Green }
function Write-WarningLine { param([string]$Message) Write-Labelled -Label "WARN" -Message $Message -Color Yellow }
function Stop-Step { param([string]$Message) Write-Labelled -Label "FAIL" -Message $Message -Color Red; exit 1 }

function Get-ManifestPath { param([string]$Root) Join-Path $Root $ManifestName }
function Test-ManagedInstall { param([string]$Root) Test-Path -LiteralPath (Get-ManifestPath -Root $Root) }
function Confirm-ManagedInstallTarget {
  param([string]$Root)
  if ((Test-Path -LiteralPath $Root) -and -not (Test-ManagedInstall -Root $Root)) {
    Stop-Step "Refusing to replace '$Root' because it is not marked as a managed LowHunt install."
  }
}

function Copy-Payload {
  param([string]$DestinationRoot)
  New-Item -ItemType Directory -Path $DestinationRoot -Force | Out-Null
  foreach ($entry in $RuntimePayload) {
    $source = Join-Path $RepoRoot $entry
    if (Test-Path -LiteralPath $source) {
      Copy-Item -LiteralPath $source -Destination $DestinationRoot -Recurse -Force
    }
  }
}

function Confirm-CommandAvailable {
  param([string]$Name)
  if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
    Stop-Step "$Name is required but was not found in PATH."
  }
}

function Invoke-Build {
  param([string]$AppRoot)
  Confirm-CommandAvailable -Name "gcc"
  $makeCmd = Get-Command "mingw32-make" -ErrorAction SilentlyContinue
  if (-not $makeCmd) { $makeCmd = Get-Command "make" -ErrorAction SilentlyContinue }
  if (-not $makeCmd) { Stop-Step "mingw32-make or make is required but was not found in PATH." }

  Write-Info "Building LowHunt inside $AppRoot"
  Push-Location $AppRoot
  try {
    & $makeCmd.Source clean *> $null
    & $makeCmd.Source
    if ($LASTEXITCODE -ne 0) {
      Stop-Step "Build failed for $AppRoot. Install libcurl development headers and try again."
    }
  } finally {
    Pop-Location
  }
}

function Write-Manifest {
  param([string]$AppRoot, [string]$InstallMode)
  $manifest = [ordered]@{
    app_name     = $AppName
    installed_at = (Get-Date).ToUniversalTime().ToString("o")
    install_mode = $InstallMode
    source_repo  = $RepoRoot
  }
  $manifest | ConvertTo-Json | Set-Content -LiteralPath (Get-ManifestPath -Root $AppRoot) -Encoding UTF8
}

function Write-Launcher {
  param([string]$AppRoot, [string]$LauncherPath)
  New-Item -ItemType Directory -Path (Split-Path -Parent $LauncherPath) -Force | Out-Null
  @"
@echo off
set "APP_ROOT=$AppRoot"
if not exist "%APP_ROOT%\lowhunt.exe" (
  echo [FAIL] LowHunt is not installed correctly at %APP_ROOT%. 1>&2
  exit /b 1
)
"%APP_ROOT%\lowhunt.exe" %*
"@ | Set-Content -LiteralPath $LauncherPath -Encoding ASCII
}

function Update-UserPath {
  if (-not (Test-Path -LiteralPath $UserBinDir)) {
    New-Item -ItemType Directory -Path $UserBinDir -Force | Out-Null
  }
  $currentUserPath = [Environment]::GetEnvironmentVariable("Path", "User")
  if ([string]::IsNullOrWhiteSpace($currentUserPath)) { $currentUserPath = "" }
  if ($currentUserPath -notlike "*$UserBinDir*") {
    $next = if ([string]::IsNullOrWhiteSpace($currentUserPath)) { $UserBinDir } else { "$currentUserPath;$UserBinDir" }
    [Environment]::SetEnvironmentVariable("Path", $next, "User")
    Write-Success "Added $UserBinDir to the user PATH."
  }
}

function Deploy-Install {
  param([string]$TargetRoot, [string]$LauncherPath, [string]$InstallMode, [bool]$UpdatePath)
  $stageRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lowhunt-install-" + [guid]::NewGuid().ToString("N"))
  $stageApp = Join-Path $stageRoot $AppName

  Confirm-ManagedInstallTarget -Root $TargetRoot
  Copy-Payload -DestinationRoot $stageApp
  Invoke-Build -AppRoot $stageApp
  Write-Manifest -AppRoot $stageApp -InstallMode $InstallMode

  if (Test-Path -LiteralPath $TargetRoot) { Remove-Item -LiteralPath $TargetRoot -Recurse -Force }
  New-Item -ItemType Directory -Path (Split-Path -Parent $TargetRoot) -Force | Out-Null
  Move-Item -LiteralPath $stageApp -Destination $TargetRoot -Force
  Write-Launcher -AppRoot $TargetRoot -LauncherPath $LauncherPath
  if ($UpdatePath) { Update-UserPath }
  if (Test-Path -LiteralPath $stageRoot) { Remove-Item -LiteralPath $stageRoot -Recurse -Force }

  Write-Success "LowHunt installed successfully."
  Write-Info "Installed application: $TargetRoot"
  Write-Info "Launcher: $LauncherPath"
}

function Uninstall-System {
  if (Test-Path -LiteralPath $InstallRoot) {
    if (-not (Test-ManagedInstall -Root $InstallRoot)) {
      Stop-Step "Refusing to remove $InstallRoot because it is not marked as managed by this installer."
    }
    Remove-Item -LiteralPath $InstallRoot -Recurse -Force
    Write-Success "Removed $InstallRoot"
  } else {
    Write-WarningLine "No managed installation was found at $InstallRoot."
  }
  if (Test-Path -LiteralPath $SystemLauncher) {
    Remove-Item -LiteralPath $SystemLauncher -Force
  }
}

switch ($Mode) {
  "install" { Deploy-Install -TargetRoot $InstallRoot -LauncherPath $SystemLauncher -InstallMode "install" -UpdatePath $true }
  "update" { Deploy-Install -TargetRoot $InstallRoot -LauncherPath $SystemLauncher -InstallMode "update" -UpdatePath $true }
  "test" { Deploy-Install -TargetRoot $TestRoot -LauncherPath $TestLauncher -InstallMode "test" -UpdatePath $false }
  "uninstall" { Uninstall-System }
}
