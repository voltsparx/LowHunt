#!/usr/bin/env bash
set -u
set -o pipefail

APP_NAME="lowhunt"
THEME_TAG="LowHunt"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INSTALL_ROOT="$HOME/.local/share/$APP_NAME"
USER_BIN_DIR="$HOME/.local/bin"
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

confirm_action() {
  local prompt="$1"
  if [ ! -t 0 ]; then return 0; fi
  printf '%s [Y/n] ' "$prompt"
  read -r answer
  case "$(printf '%s' "$answer" | tr '[:upper:]' '[:lower:]')" in
    ""|y|yes) return 0 ;;
    *) return 1 ;;
  esac
}

manifest_path() { printf '%s/%s\n' "$1" "$MANIFEST_NAME"; }
is_managed_install() { [ -f "$(manifest_path "$1")" ]; }
ensure_managed_or_missing() { [ ! -e "$1" ] || is_managed_install "$1" || fail "Refusing to replace '$1' because it is not marked as a managed LowHunt install."; }
ensure_command() { command -v "$1" >/dev/null 2>&1 || fail "$1 is required but was not found in PATH."; }

copy_payload() {
  local destination_root="$1"
  run_cmd mkdir -p "$destination_root"
  local entry
  for entry in "${RUNTIME_PAYLOAD[@]}"; do
    [ -e "$REPO_ROOT/$entry" ] || continue
    run_cmd cp -R "$REPO_ROOT/$entry" "$destination_root/"
  done
}

compile_app() {
  local app_root="$1"
  ensure_command gcc
  ensure_command make
  info "Building LowHunt inside $app_root"
  (
    cd "$app_root" || exit 1
    make clean >/dev/null 2>&1 || true
    make
  )
  local code=$?
  [ "$code" -eq 0 ] || fail "Build failed for $app_root. Install libcurl development headers and try again."
}

write_manifest() {
  local app_root="$1"
  local install_mode="$2"
  cat >"$(manifest_path "$app_root")" <<EOF
{
  "app_name": "$APP_NAME",
  "installed_at": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "install_mode": "$install_mode",
  "source_repo": "$REPO_ROOT"
}
EOF
}

write_launcher() {
  local app_root="$1"
  local launcher_path="$2"
  run_cmd mkdir -p "$(dirname "$launcher_path")"
  cat >"$launcher_path" <<EOF
#!/usr/bin/env sh
APP_ROOT="$app_root"
if [ ! -x "\$APP_ROOT/lowhunt" ]; then
  echo "[FAIL] LowHunt is not installed correctly at \$APP_ROOT." >&2
  exit 1
fi
exec "\$APP_ROOT/lowhunt" "\$@"
EOF
  run_cmd chmod +x "$launcher_path"
}

ensure_path_and_man_blocks() {
  run_cmd mkdir -p "$USER_BIN_DIR"
  local marker_start="# >>> lowhunt >>>"
  local marker_end="# <<< lowhunt <<<"
  local profile written="no"

  for profile in "${PROFILE_FILES[@]}"; do
    [ -f "$profile" ] || : >"$profile" || continue
    grep -Fq "$marker_start" "$profile" 2>/dev/null && continue
    {
      printf '\n%s\n' "$marker_start"
      printf '%s\n' 'export PATH="$HOME/.local/bin:$PATH"'
      printf '%s\n' "export MANPATH=\"$INSTALL_ROOT/man:\${MANPATH:-}\""
      printf '%s\n' "$marker_end"
    } >>"$profile" || continue
    written="yes"
  done
  [ "$written" = "yes" ] && ok "Added PATH and MANPATH updates to common shell profiles."
}

remove_profile_block() {
  local marker_start="# >>> lowhunt >>>"
  local marker_end="# <<< lowhunt <<<"
  local profile
  for profile in "${PROFILE_FILES[@]}"; do
    [ -f "$profile" ] || continue
    awk -v start="$marker_start" -v end="$marker_end" '
      $0 == start { skip = 1; next }
      $0 == end { skip = 0; next }
      !skip { print }
    ' "$profile" >"$profile.tmp" && mv "$profile.tmp" "$profile"
  done
}

show_install_summary() {
  local app_root="$1"
  ok "LowHunt installed successfully."
  info "Installed application: $app_root"
  info "Launcher: $SYSTEM_LAUNCHER"
  info "Manual path: $app_root/man"
}

deploy_install() {
  local target_root="$1"
  local launcher_path="$2"
  local install_mode="$3"
  local update_profiles="$4"
  local stage_root
  stage_root="$(mktemp -d "${TMPDIR:-/tmp}/lowhunt-install.XXXXXX")"
  local stage_app="$stage_root/$APP_NAME"

  ensure_managed_or_missing "$target_root"
  copy_payload "$stage_app"
  compile_app "$stage_app"
  write_manifest "$stage_app" "$install_mode"
  run_cmd rm -rf "$target_root"
  run_cmd mkdir -p "$(dirname "$target_root")"
  run_cmd mv "$stage_app" "$target_root"
  write_launcher "$target_root" "$launcher_path"
  [ "$update_profiles" = "yes" ] && ensure_path_and_man_blocks
  run_cmd rm -rf "$stage_root"
  show_install_summary "$target_root"
}

uninstall_system() {
  if [ -d "$INSTALL_ROOT" ]; then
    is_managed_install "$INSTALL_ROOT" || fail "Refusing to remove $INSTALL_ROOT because it is not marked as managed by this installer."
    run_cmd rm -rf "$INSTALL_ROOT"
    ok "Removed $INSTALL_ROOT"
  else
    warn "No managed installation was found at $INSTALL_ROOT."
  fi
  run_cmd rm -f "$SYSTEM_LAUNCHER"
  remove_profile_block
}

case "$MODE" in
  install) deploy_install "$INSTALL_ROOT" "$SYSTEM_LAUNCHER" "install" "yes" ;;
  update) deploy_install "$INSTALL_ROOT" "$SYSTEM_LAUNCHER" "update" "yes" ;;
  test) deploy_install "$TEST_ROOT" "$TEST_LAUNCHER" "test" "no" ;;
  uninstall) uninstall_system ;;
  *) fail "Unsupported mode '$MODE'. Use install, update, uninstall, or test." ;;
esac
