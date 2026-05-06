param(
  [string]$Root = (Resolve-Path "$PSScriptRoot/..").Path
)

$ErrorActionPreference = "Stop"

$tmpTest = Join-Path $Root "tmp/test-run"
New-Item -ItemType Directory -Force -Path $tmpTest | Out-Null

& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tests/validate_platforms.ps1") -Root $Root

$syntaxLog = Join-Path $tmpTest "syntax.log"
$commonSources = @(
  "src/app/main.c",
  "src/config/config.c",
  "src/core/utils.c",
  "src/core/metadata.c",
  "src/core/resource_guard.c",
  "src/ui/banner.c",
  "src/ui/help_menu.c",
  "src/net/scanner.c",
  "src/net/harvester.c",
  "src/output/output.c",
  "src/engines/engine_loader.c",
  "src/engines/parallel_engine.c",
  "src/engines/threadpool_engine.c",
  "src/engines/sync_engine.c",
  "src/engines/async_engine.c",
  "src/engines/fusion_engine.c",
  "src/engines/stabilizer_engine.c",
  "src/engines/intelligence_engine.c",
  "src/engines/brief_report_engine.c",
  "tests/test_scanner.c",
  "tests/test_output.c"
)

if (Get-Command gcc -ErrorAction SilentlyContinue) {
  $sources = @($commonSources)
  $curlProbe = Join-Path $tmpTest "curl_probe.c"
  Set-Content -LiteralPath $curlProbe -Encoding ASCII -Value "#include <curl/curl.h>"
  $previousPreference = $ErrorActionPreference
  $ErrorActionPreference = "Continue"
  & gcc -std=c11 -E -Isrc/include $curlProbe *> $null
  $ErrorActionPreference = $previousPreference
  if ($LASTEXITCODE -eq 0) {
    $sources += "src/net/http.c"
  } else {
    Write-Host "libcurl headers not found; skipped syntax check for src/net/http.c."
  }

  & gcc -std=c11 -Wall -Wextra -Isrc/include -fsyntax-only @sources *> $syntaxLog
  if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $syntaxLog
    throw "C syntax check failed"
  }
  Write-Host "C syntax check passed."
} else {
  Write-Host "gcc not found; skipped C syntax check."
}

Write-Host "All available tests passed. Artifacts: $tmpTest"
