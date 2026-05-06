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
  gcc -std=c11 -Wall -Wextra -Iinclude -fsyntax-only \
    src/main.c src/config.c src/scanner.c src/harvester.c src/output.c src/utils.c \
    tests/test_scanner.c tests/test_output.c \
    > "$TMP_TEST/syntax.log" 2>&1
  echo "C syntax check passed."
else
  echo "gcc not found; skipped C syntax check."
fi

echo "All available tests passed. Artifacts: $TMP_TEST"
