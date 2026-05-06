#!/data/data/com.termux/files/usr/bin/bash
set -u
set -o pipefail

APP_NAME="lowhunt"
THEME_TAG="LowHunt"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INSTALL_ROOT="$HOME/.local/share/$APP_NAME"
USER_BIN_DIR="${PREFIX:-$HOME/.local}/bin"
SYSTEM_LAUNCHER="$USER_BIN_DIR/$APP_NAME"
TEST_BASE="$SCRIPT_DIR/test-root"
TEST_ROOT="$TEST_BASE/$APP_NAME"
TEST_BIN_DIR="$TEST_BASE/bin"
TEST_LAUNCHER="$TEST_BIN_DIR/$APP_NAME"
MANIFEST_NAME=".lowhunt-install.json"
MODE="${1:-install}"
PROFILE_FILES=("$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.profile")
RUNTIME_PAYLOAD=("src" "data" "platforms" "tests" "man" "README.md" "LICENSE" "Makefile" "CMakeLists.txt")

log() { printf '[%s][%s] %s\n' "$THEME_TAG" "$1" "$2"; }
info() { log "INFO" "$1"; }
ok() { log " OK " "$1"; }
warn() { log "WARN" "$1"; }
fail() { log "FAIL" "$1"; exit 1; }
run_cmd() { "$@"; local code=$?; [ "$code" -eq 0 ] || fail "Command failed ($code): $*"; }
manifest_path() { printf '%s/%s\n' "$1" "$MANIFEST_NAME"; }
is_managed_install() { [ -f "$(manifest_path "$1")" ]; }
ensure_managed_or_missing() { [ ! -e "$1" ] || is_managed_install "$1" || fail "Refusing to replace '$1' because it is not marked as a managed LowHunt install."; }
ensure_command() { command -v "$1" >/dev/null 2>&1 || fail "$1 is required but was not found in PATH."; }

copy_payload() { local d="$1"; run_cmd mkdir -p "$d"; local e; for e in "${RUNTIME_PAYLOAD[@]}"; do [ -e "$REPO_ROOT/$e" ] && run_cmd cp -R "$REPO_ROOT/$e" "$d/"; done; }
compile_app() { local a="$1"; ensure_command gcc; ensure_command make; info "Building LowHunt inside $a"; ( cd "$a" || exit 1; make clean >/dev/null 2>&1 || true; make ) || fail "Build failed for $a. Install curl and build tools in Termux first."; }
write_manifest() { local a="$1"; local m="$2"; cat >"$(manifest_path "$a")" <<EOF
{"app_name":"$APP_NAME","installed_at":"$(date -u +"%Y-%m-%dT%H:%M:%SZ")","install_mode":"$m","source_repo":"$REPO_ROOT"}
EOF
}
write_launcher() { local a="$1"; local l="$2"; run_cmd mkdir -p "$(dirname "$l")"; cat >"$l" <<EOF
#!/data/data/com.termux/files/usr/bin/sh
APP_ROOT="$a"
[ -x "\$APP_ROOT/lowhunt" ] || { echo "[FAIL] LowHunt is not installed correctly at \$APP_ROOT." >&2; exit 1; }
exec "\$APP_ROOT/lowhunt" "\$@"
EOF
run_cmd chmod +x "$l"; }
ensure_path_and_man_blocks() { local s="# >>> lowhunt >>>"; local e="# <<< lowhunt <<<"; local p w="no"; for p in "${PROFILE_FILES[@]}"; do [ -f "$p" ] || : >"$p" || continue; grep -Fq "$s" "$p" 2>/dev/null && continue; { printf '\n%s\n' "$s"; printf '%s\n' "export PATH=\"$USER_BIN_DIR:\$PATH\""; printf '%s\n' "export MANPATH=\"$INSTALL_ROOT/man:\${MANPATH:-}\""; printf '%s\n' "$e"; } >>"$p" || continue; w="yes"; done; [ "$w" = "yes" ] && ok "Added PATH and MANPATH updates to shell profiles."; }
remove_profile_block() { local s="# >>> lowhunt >>>"; local e="# <<< lowhunt <<<"; local p; for p in "${PROFILE_FILES[@]}"; do [ -f "$p" ] || continue; awk -v start="$s" -v end="$e" '$0 == start { skip = 1; next } $0 == end { skip = 0; next } !skip { print }' "$p" >"$p.tmp" && mv "$p.tmp" "$p"; done; }
deploy_install() { local t="$1"; local l="$2"; local m="$3"; local u="$4"; local stage_root; stage_root="$(mktemp -d "${TMPDIR:-/data/data/com.termux/files/usr/tmp}/lowhunt-install.XXXXXX")"; local stage_app="$stage_root/$APP_NAME"; ensure_managed_or_missing "$t"; copy_payload "$stage_app"; compile_app "$stage_app"; write_manifest "$stage_app" "$m"; run_cmd rm -rf "$t"; run_cmd mkdir -p "$(dirname "$t")"; run_cmd mv "$stage_app" "$t"; write_launcher "$t" "$l"; [ "$u" = "yes" ] && ensure_path_and_man_blocks; run_cmd rm -rf "$stage_root"; ok "LowHunt installed successfully."; info "Installed application: $t"; info "Launcher: $l"; }
uninstall_system() { if [ -d "$INSTALL_ROOT" ]; then is_managed_install "$INSTALL_ROOT" || fail "Refusing to remove $INSTALL_ROOT because it is not marked as managed by this installer."; run_cmd rm -rf "$INSTALL_ROOT"; ok "Removed $INSTALL_ROOT"; else warn "No managed installation was found at $INSTALL_ROOT."; fi; run_cmd rm -f "$SYSTEM_LAUNCHER"; remove_profile_block; }

case "$MODE" in
  install) deploy_install "$INSTALL_ROOT" "$SYSTEM_LAUNCHER" "install" "yes" ;;
  update) deploy_install "$INSTALL_ROOT" "$SYSTEM_LAUNCHER" "update" "yes" ;;
  test) deploy_install "$TEST_ROOT" "$TEST_LAUNCHER" "test" "no" ;;
  uninstall) uninstall_system ;;
  *) fail "Unsupported mode '$MODE'. Use install, update, uninstall, or test." ;;
esac
