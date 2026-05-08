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
$UserStateRoot = Join-Path $HOME ".lowhunt"
$DefaultOutputRoot = Join-Path $UserStateRoot "output"
$ManifestName = ".lowhunt-install.json"
$RuntimePayload = @("src", "data", "platforms", "tests", "man", "README.md", "LICENSE", "Makefile", "CMakeLists.txt")

function Write-Labelled {
  param(
    [string]$Label,
    [string]$Message,
    [ConsoleColor]$Color = [ConsoleColor]::Gray
  )

  Write-Host ("[{0}][{1}] {2}" -f $ThemeTag, $Label, $Message) -ForegroundColor $Color
}

function Write-Info { param([string]$Message) Write-Labelled -Label "INFO" -Message $Message -Color ([ConsoleColor]::Cyan) }
function Write-Success { param([string]$Message) Write-Labelled -Label " OK " -Message $Message -Color ([ConsoleColor]::Green) }
function Write-WarningLine { param([string]$Message) Write-Labelled -Label "WARN" -Message $Message -Color ([ConsoleColor]::Yellow) }
function Stop-Step { param([string]$Message) Write-Labelled -Label "FAIL" -Message $Message -Color ([ConsoleColor]::Red); exit 1 }

function Confirm-CommandAvailable {
  param([string]$Name)
  if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
    Stop-Step "$Name is required but was not found in PATH."
  }
}

function Get-ManifestPath {
  param([string]$Root)
  Join-Path $Root $ManifestName
}

function Test-ManagedInstall {
  param([string]$Root)
  Test-Path -LiteralPath (Get-ManifestPath -Root $Root)
}

function Confirm-ManagedInstallTarget {
  param([string]$Root)
  if ((Test-Path -LiteralPath $Root) -and -not (Test-ManagedInstall -Root $Root)) {
    Stop-Step "Refusing to replace '$Root' because it is not marked as a managed LowHunt install."
  }
}

function Remove-TreeSafe {
  param([string]$PathToRemove)

  if ([string]::IsNullOrWhiteSpace($PathToRemove) -or -not (Test-Path -LiteralPath $PathToRemove)) {
    return $true
  }

  foreach ($attempt in 1..5) {
    try {
      Remove-Item -LiteralPath $PathToRemove -Recurse -Force -ErrorAction Stop
      return $true
    } catch {
      if ($attempt -lt 5) {
        Start-Sleep -Milliseconds 300
      }
    }
  }

  Write-WarningLine "Unable to remove path immediately: $PathToRemove"
  return $false
}

function Promote-StagedInstall {
  param(
    [string]$StageApp,
    [string]$TargetRoot
  )

  foreach ($attempt in 1..5) {
    try {
      Move-Item -LiteralPath $StageApp -Destination $TargetRoot -Force -ErrorAction Stop
      return
    } catch {
      if ($attempt -lt 5) {
        Start-Sleep -Milliseconds 350
      }
    }
  }

  Write-WarningLine "Direct stage move failed, falling back to a file-level copy."
  New-Item -ItemType Directory -Path $TargetRoot -Force | Out-Null
  Get-ChildItem -LiteralPath $StageApp -Force -ErrorAction Stop | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $TargetRoot -Recurse -Force -ErrorAction Stop
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

  foreach ($dir in @("output", "tmp")) {
    New-Item -ItemType Directory -Path (Join-Path $DestinationRoot $dir) -Force | Out-Null
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
      Stop-Step "Build failed for $AppRoot. Install MinGW make and libcurl development headers, then try again."
    }
  } finally {
    Pop-Location
  }
}

function Write-Manifest {
  param(
    [string]$AppRoot,
    [string]$InstallMode
  )

  $manifest = [ordered]@{
    app_name     = $AppName
    installed_at = (Get-Date).ToUniversalTime().ToString("o")
    install_mode = $InstallMode
    source_repo  = $RepoRoot
  }

  $manifest | ConvertTo-Json | Set-Content -LiteralPath (Get-ManifestPath -Root $AppRoot) -Encoding UTF8
}

function Write-UserState {
  New-Item -ItemType Directory -Path $UserStateRoot -Force | Out-Null
  New-Item -ItemType Directory -Path $DefaultOutputRoot -Force | Out-Null
}

function Write-Launcher {
  param(
    [string]$AppRoot,
    [string]$LauncherPath
  )

  New-Item -ItemType Directory -Path (Split-Path -Parent $LauncherPath) -Force | Out-Null
  @"
@echo off
setlocal
set "APP_ROOT=$AppRoot"
if not exist "%APP_ROOT%\lowhunt.exe" (
  echo [FAIL] LowHunt is not installed correctly at %APP_ROOT%.
  exit /b 1
)
"%APP_ROOT%\lowhunt.exe" %*
"@ | Set-Content -LiteralPath $LauncherPath -Encoding ASCII
}

function Add-UserPathEntry {
  param([string]$Directory)

  New-Item -ItemType Directory -Path $Directory -Force | Out-Null
  $current = [Environment]::GetEnvironmentVariable("Path", "User")
  $entries = @()
  if (-not [string]::IsNullOrWhiteSpace($current)) {
    $entries = $current.Split(";") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
  }

  $alreadyPresent = $false
  foreach ($entry in $entries) {
    if ($entry.TrimEnd("\") -ieq $Directory.TrimEnd("\")) {
      $alreadyPresent = $true
      break
    }
  }

  if (-not $alreadyPresent) {
    $newPath = (($entries + $Directory) | Select-Object -Unique) -join ";"
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    if ($env:Path -notlike "*$Directory*") {
      $env:Path = "$Directory;$env:Path"
    }
    Write-Success "Added $Directory to the user PATH."
  } else {
    Write-Info "$Directory is already present in the user PATH."
  }
}

function Remove-UserPathEntry {
  param([string]$Directory)

  $current = [Environment]::GetEnvironmentVariable("Path", "User")
  if ([string]::IsNullOrWhiteSpace($current)) {
    return
  }

  $entries = $current.Split(";") | Where-Object {
    -not [string]::IsNullOrWhiteSpace($_) -and $_.TrimEnd("\") -ine $Directory.TrimEnd("\")
  }

  [Environment]::SetEnvironmentVariable("Path", ($entries -join ";"), "User")
}

function Invoke-SmokeTest {
  param([string]$LauncherPath)

  Write-Info "Running a launcher smoke test."
  & cmd.exe /c "`"$LauncherPath`" --version" *> $null
  if ($LASTEXITCODE -ne 0) {
    Stop-Step "Smoke test failed for $LauncherPath."
  }
  Write-Success "Launcher smoke test passed."
}

function Show-InstallSummary {
  param(
    [string]$InstallMode,
    [string]$AppRoot,
    [string]$LauncherPath
  )

  Write-Success "LowHunt $InstallMode completed successfully."
  Write-Info "Installed application: $AppRoot"
  Write-Info "Launcher: $LauncherPath"
  Write-Info "Default output root: $DefaultOutputRoot"
  Write-Info "Manual command: man lowhunt (WSL/MSYS) or lowhunt --help"
}

function Install-Application {
  param(
    [string]$TargetRoot,
    [string]$LauncherPath,
    [string]$InstallMode,
    [switch]$AddToPath
  )

  Confirm-ManagedInstallTarget -Root $TargetRoot

  $parent = Split-Path -Parent $TargetRoot
  $stageRoot = Join-Path $parent ".$AppName-staging-$PID"
  $stageApp = Join-Path $stageRoot $AppName
  $backupRoot = Join-Path $parent ".$AppName-backup-$PID"
  $restored = $false

  $null = Remove-TreeSafe -PathToRemove $stageRoot
  $null = Remove-TreeSafe -PathToRemove $backupRoot
  New-Item -ItemType Directory -Path $stageRoot -Force | Out-Null

  try {
    Write-Info "Preparing staged files for $InstallMode."
    Copy-Payload -DestinationRoot $stageApp
    Invoke-Build -AppRoot $stageApp
    Write-Manifest -AppRoot $stageApp -InstallMode $InstallMode

    if (Test-Path -LiteralPath $TargetRoot) {
      Move-Item -LiteralPath $TargetRoot -Destination $backupRoot -Force
    }

    Promote-StagedInstall -StageApp $stageApp -TargetRoot $TargetRoot
    Write-Launcher -AppRoot $TargetRoot -LauncherPath $LauncherPath

    if ($AddToPath) {
      Add-UserPathEntry -Directory $UserBinDir
      Write-UserState
    }

    Invoke-SmokeTest -LauncherPath $LauncherPath
    $null = Remove-TreeSafe -PathToRemove $backupRoot
    $null = Remove-TreeSafe -PathToRemove $stageRoot
    Show-InstallSummary -InstallMode $InstallMode -AppRoot $TargetRoot -LauncherPath $LauncherPath
  } catch {
    $originalError = $_.Exception.Message
    Write-WarningLine "Attempting to restore the previous installation state."
    if (Test-Path -LiteralPath $backupRoot) {
      if (Test-Path -LiteralPath $TargetRoot) {
        $null = Remove-TreeSafe -PathToRemove $TargetRoot
      }
      if (-not (Test-Path -LiteralPath $TargetRoot)) {
        Move-Item -LiteralPath $backupRoot -Destination $TargetRoot -Force
        $restored = $true
      }
    }

    $null = Remove-TreeSafe -PathToRemove $stageRoot
    if (-not $restored) {
      $null = Remove-TreeSafe -PathToRemove $backupRoot
    }
    Stop-Step $originalError
  }
}

function Uninstall-Application {
  if (-not (Test-Path -LiteralPath $InstallRoot)) {
    Write-WarningLine "No managed installation was found at $InstallRoot."
  } elseif (-not (Test-ManagedInstall -Root $InstallRoot)) {
    Stop-Step "Refusing to remove $InstallRoot because it is not marked as managed by this installer."
  } else {
    Remove-TreeSafe -PathToRemove $InstallRoot | Out-Null
    Write-Success "Removed $InstallRoot"
  }

  if (Test-Path -LiteralPath $SystemLauncher) {
    Remove-Item -LiteralPath $SystemLauncher -Force
    Write-Success "Removed launcher $SystemLauncher"
  }

  Remove-UserPathEntry -Directory $UserBinDir
  Write-Success "User PATH cleaned up."
}

switch ($Mode) {
  "install" {
    Install-Application -TargetRoot $InstallRoot -LauncherPath $SystemLauncher -InstallMode "install" -AddToPath
  }
  "test" {
    Install-Application -TargetRoot $TestRoot -LauncherPath $TestLauncher -InstallMode "test"
    Write-Info "Repo-local test launcher: $TestLauncher"
  }
  "update" {
    if (-not (Test-ManagedInstall -Root $InstallRoot)) {
      Stop-Step "No managed installation was found to update. Run install first."
    }
    Install-Application -TargetRoot $InstallRoot -LauncherPath $SystemLauncher -InstallMode "update" -AddToPath
  }
  "uninstall" {
    Uninstall-Application
  }
}
