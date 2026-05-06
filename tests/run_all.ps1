param(
  [string]$Root = (Resolve-Path "$PSScriptRoot/..").Path
)

$ErrorActionPreference = "Stop"

$tmpTest = Join-Path $Root "tmp/test-run"
New-Item -ItemType Directory -Force -Path $tmpTest | Out-Null

& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tests/validate_platforms.ps1") -Root $Root

$syntaxLog = Join-Path $tmpTest "syntax.log"
$sources = @(
  "src/main.c",
  "src/config.c",
  "src/scanner.c",
  "src/harvester.c",
  "src/output.c",
  "src/utils.c",
  "tests/test_scanner.c",
  "tests/test_output.c"
)

if (Get-Command gcc -ErrorAction SilentlyContinue) {
  & gcc -std=c11 -Wall -Wextra -Iinclude -fsyntax-only @sources *> $syntaxLog
  if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $syntaxLog
    throw "C syntax check failed"
  }
  Write-Host "C syntax check passed."
} else {
  Write-Host "gcc not found; skipped C syntax check."
}

Write-Host "All available tests passed. Artifacts: $tmpTest"
