#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP_TEST="$ROOT/tmp/test-run"
mkdir -p "$TMP_TEST"

python3 - <<'PY' "$ROOT"
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
platforms = sorted((root / "platforms").glob("*.json"))
if len(platforms) < 100:
    raise SystemExit(f"expected at least 100 platform manifests, found {len(platforms)}")
names = set()
for path in platforms:
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    for field in ("name", "url", "errorType", "category", "isNSFW"):
        if field not in data:
            raise SystemExit(f"{path.name} missing required field {field}")
    if "{}" not in data["url"]:
        raise SystemExit(f"{path.name} url does not contain {{}} username token")
    name = data["name"].lower()
    if name in names:
        raise SystemExit(f"duplicate platform name {data['name']}")
    names.add(name)
print(f"Validated {len(platforms)} platform manifests.")
PY

if command -v gcc >/dev/null 2>&1; then
  SOURCES=(
    src/app/main.c src/config/config.c src/core/utils.c src/core/metadata.c src/core/resource_guard.c
    src/ui/banner.c src/ui/help_menu.c
    src/net/scanner.c src/net/harvester.c src/output/output.c
    src/engines/engine_loader.c src/engines/parallel_engine.c src/engines/threadpool_engine.c
    src/engines/sync_engine.c src/engines/async_engine.c src/engines/fusion_engine.c
    src/engines/stabilizer_engine.c src/engines/intelligence_engine.c src/engines/brief_report_engine.c
    tests/test_scanner.c tests/test_output.c
  )

  if printf '#include <curl/curl.h>\n' | gcc -std=c11 -E -Isrc/include -xc - >/dev/null 2>&1; then
    SOURCES+=(src/net/http.c)
  else
    echo "libcurl headers not found; skipped syntax check for src/net/http.c."
  fi

  gcc -std=c11 -Wall -Wextra -Isrc/include -fsyntax-only \
    "${SOURCES[@]}" \
    > "$TMP_TEST/syntax.log" 2>&1
  echo "C syntax check passed."
else
  echo "gcc not found; skipped C syntax check."
fi

echo "All available tests passed. Artifacts: $TMP_TEST"
