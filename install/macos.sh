#!/usr/bin/env bash
set -u
set -o pipefail

APP_NAME="lowhunt"
THEME_TAG="LowHunt"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INSTALL_ROOT="$HOME/Library/Application Support/$APP_NAME"
USER_BIN_DIR="$HOME/.local/bin"
SYSTEM_LAUNCHER="$USER_BIN_DIR/$APP_NAME"
TEST_BASE="$SCRIPT_DIR/test-root"
TEST_ROOT="$TEST_BASE/$APP_NAME"
TEST_BIN_DIR="$TEST_BASE/bin"
TEST_LAUNCHER="$TEST_BIN_DIR/$APP_NAME"
USER_STATE_ROOT="$HOME/.lowhunt"
DEFAULT_OUTPUT_ROOT="$USER_STATE_ROOT/output"
MANIFEST_NAME=".lowhunt-install.json"
MODE="${1:-install}"
PROFILE_FILES=("$HOME/.zshrc" "$HOME/.bash_profile" "$HOME/.bashrc" "$HOME/.profile")
RUNTIME_PAYLOAD=("src" "data" "platforms" "tests" "man" "README.md" "LICENSE" "Makefile" "CMakeLists.txt")

log() { printf '[%s][%s] %s\n' "$THEME_TAG" "$1" "$2"; }
info() { log "INFO" "$1"; }
ok() { log " OK " "$1"; }
warn() { log "WARN" "$1"; }
fail() { log "FAIL" "$1"; exit 1; }

run_cmd() {
  "$@"
  local code=$?
  [ "$code" -eq 0 ] || fail "Command failed ($code): $*"
}

ensure_command() {
  command -v "$1" >/dev/null 2>&1 || fail "$1 is required but was not found in PATH."
}

manifest_path() { printf '%s/%s\n' "$1" "$MANIFEST_NAME"; }
is_managed_install() { [ -f "$(manifest_path "$1")" ]; }

ensure_managed_or_missing() {
  if [ -e "$1" ] && ! is_managed_install "$1"; then
    fail "Refusing to replace '$1' because it is not marked as a managed LowHunt install."
  fi
}

remove_tree_safe() {
  local path="$1"
  [ -n "$path" ] || return 0
  [ -e "$path" ] || return 0
  rm -rf "$path" || fail "Unable to remove: $path"
}

copy_payload() {
  local destination_root="$1"

  run_cmd mkdir -p "$destination_root"
  local entry
  for entry in "${RUNTIME_PAYLOAD[@]}"; do
    [ -e "$REPO_ROOT/$entry" ] || continue
    run_cmd cp -R "$REPO_ROOT/$entry" "$destination_root/"
  done

  run_cmd mkdir -p "$destination_root/output" "$destination_root/tmp"
}

compile_app() {
  local app_root="$1"
  ensure_command clang
  ensure_command make

  info "Building LowHunt inside $app_root"
  (
    cd "$app_root" || exit 1
    make clean >/dev/null 2>&1 || true
    CC=clang make
  )
  local code=$?
  [ "$code" -eq 0 ] || fail "Build failed for $app_root. Install Xcode Command Line Tools and libcurl, then try again."
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

write_user_state() {
  run_cmd mkdir -p "$USER_STATE_ROOT" "$DEFAULT_OUTPUT_ROOT"
}

write_launcher() {
  local app_root="$1"
  local launcher_path="$2"

  run_cmd mkdir -p "$(dirname "$launcher_path")"
  cat >"$launcher_path" <<EOF
#!/usr/bin/env sh
APP_ROOT="$app_root"
[ -x "\$APP_ROOT/lowhunt" ] || { echo "[FAIL] LowHunt is not installed correctly at \$APP_ROOT." >&2; exit 1; }
exec "\$APP_ROOT/lowhunt" "\$@"
EOF
  run_cmd chmod +x "$launcher_path"
}

ensure_path_and_man_blocks() {
  run_cmd mkdir -p "$USER_BIN_DIR"
  local marker_start="# >>> lowhunt >>>"
  local marker_end="# <<< lowhunt <<<"
  local profile
  local written="no"

  for profile in "${PROFILE_FILES[@]}"; do
    [ -f "$profile" ] || : >"$profile" || continue
    if grep -Fq "$marker_start" "$profile" 2>/dev/null; then
      continue
    fi
    {
      printf '\n%s\n' "$marker_start"
      printf '%s\n' 'export PATH="$HOME/.local/bin:$PATH"'
      printf '%s\n' "export MANPATH=\"$INSTALL_ROOT/man:\${MANPATH:-}\""
      printf '%s\n' "$marker_end"
    } >>"$profile" || continue
    written="yes"
  done

  if [ "$written" = "yes" ]; then
    ok "Added PATH and MANPATH updates to common shell profiles."
  else
    info "Shell profile updates were already present."
  fi
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

smoke_test() {
  local launcher_path="$1"
  info "Running a launcher smoke test."
  "$launcher_path" --version >/dev/null 2>&1 || fail "Smoke test failed for $launcher_path."
  ok "Launcher smoke test passed."
}

show_install_summary() {
  local install_mode="$1"
  local app_root="$2"
  local launcher_path="$3"

  ok "LowHunt $install_mode completed successfully."
  info "Installed application: $app_root"
  info "Launcher: $launcher_path"
  info "Default output root: $DEFAULT_OUTPUT_ROOT"
  info "Man page path: $app_root/man"
  info "Reload your shell or run: export PATH=\"$USER_BIN_DIR:\$PATH\" && export MANPATH=\"$app_root/man:\${MANPATH:-}\""
}

deploy_install() {
  local target_root="$1"
  local launcher_path="$2"
  local install_mode="$3"
  local add_to_profile="$4"
  local parent_dir
  local stage_root
  local stage_app
  local backup_root

  ensure_managed_or_missing "$target_root"
  parent_dir="$(dirname "$target_root")"
  stage_root="$parent_dir/.${APP_NAME}-staging-$$"
  stage_app="$stage_root/$APP_NAME"
  backup_root="$parent_dir/.${APP_NAME}-backup-$$"

  remove_tree_safe "$stage_root"
  remove_tree_safe "$backup_root"
  run_cmd mkdir -p "$stage_root"

  info "Preparing staged files for $install_mode."
  copy_payload "$stage_app"
  compile_app "$stage_app"
  write_manifest "$stage_app" "$install_mode"

  [ -d "$target_root" ] && run_cmd mv "$target_root" "$backup_root"
  if ! mv "$stage_app" "$target_root"; then
    [ -d "$backup_root" ] && mv "$backup_root" "$target_root"
    fail "Unable to move the staged install into place."
  fi

  if ! write_launcher "$target_root" "$launcher_path"; then
    remove_tree_safe "$target_root"
    [ -d "$backup_root" ] && mv "$backup_root" "$target_root"
    fail "Unable to create launcher at $launcher_path."
  fi

  if [ "$add_to_profile" = "yes" ]; then
    ensure_path_and_man_blocks
    write_user_state
  fi

  smoke_test "$launcher_path"
  remove_tree_safe "$backup_root"
  remove_tree_safe "$stage_root"
  show_install_summary "$install_mode" "$target_root" "$launcher_path"
}

uninstall_system() {
  if [ -d "$INSTALL_ROOT" ]; then
    is_managed_install "$INSTALL_ROOT" || fail "Refusing to remove $INSTALL_ROOT because it is not marked as managed by this installer."
    run_cmd rm -rf "$INSTALL_ROOT"
    ok "Removed $INSTALL_ROOT"
  else
    warn "No managed installation was found at $INSTALL_ROOT."
  fi

  [ -f "$SYSTEM_LAUNCHER" ] && run_cmd rm -f "$SYSTEM_LAUNCHER" && ok "Removed launcher $SYSTEM_LAUNCHER"
  remove_profile_block
  ok "Shell profile updates were removed."
}

case "$MODE" in
  install) deploy_install "$INSTALL_ROOT" "$SYSTEM_LAUNCHER" "install" "yes" ;;
  test)
    deploy_install "$TEST_ROOT" "$TEST_LAUNCHER" "test" "no"
    info "Repo-local test launcher: $TEST_LAUNCHER"
    ;;
  update)
    is_managed_install "$INSTALL_ROOT" || fail "No managed installation was found to update. Run install first."
    deploy_install "$INSTALL_ROOT" "$SYSTEM_LAUNCHER" "update" "yes"
    ;;
  uninstall) uninstall_system ;;
  *) fail "Unsupported mode '$MODE'. Use install, test, uninstall, or update." ;;
esac
