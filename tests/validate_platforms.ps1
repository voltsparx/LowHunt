param(
  [string]$Root = (Resolve-Path "$PSScriptRoot/..").Path
)

$ErrorActionPreference = "Stop"

$platformDir = Join-Path $Root "platforms"
if (-not (Test-Path -LiteralPath $platformDir)) {
  throw "platforms directory is missing"
}

$files = Get-ChildItem -LiteralPath $platformDir -Filter "*.json" -File
if ($files.Count -lt 100) {
  throw "expected at least 100 platform manifests, found $($files.Count)"
}

$names = @{}
foreach ($file in $files) {
  $json = Get-Content -Raw -LiteralPath $file.FullName | ConvertFrom-Json
  foreach ($field in @("name", "url", "errorType", "category", "isNSFW")) {
    if ($null -eq $json.$field) {
      throw "$($file.Name) is missing required field '$field'"
    }
  }
  if ($json.url -notmatch '\{\}') {
    throw "$($file.Name) url does not contain {} username token"
  }
  if ($names.ContainsKey($json.name.ToLowerInvariant())) {
    throw "duplicate platform name '$($json.name)' in $($file.Name)"
  }
  $names[$json.name.ToLowerInvariant()] = $true
}

Write-Host "Validated $($files.Count) platform manifests."
