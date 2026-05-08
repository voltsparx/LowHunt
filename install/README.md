# `install/`

Managed local installers for LowHunt live here.

## Purpose

These scripts install LowHunt like a normal local tool while keeping the
application files in a managed per-user directory. They use staged deployment,
write a manifest so updates and uninstall stay safe, and create launchers in a
user bin directory instead of modifying system-wide locations.

Supported platforms:

- Windows
- Linux
- macOS
- Termux

## Supported modes

Each installer supports the same lifecycle modes:

- `install`: stage, build, smoke-test, and install LowHunt
- `update`: refresh an existing managed install from the current repo
- `uninstall`: remove a managed install safely
- `test`: create a repo-local staged install for installer validation

## Notes

- managed installs refuse to overwrite directories not created by these scripts
- shell installers add `PATH` and `MANPATH` blocks to common profile files
- Windows adds the launcher directory to the user `PATH`
- launchers run a quick smoke test during install by calling `--version`
- `test-root/` is generated installer output and should not be treated as
  source code

## Examples

```sh
./install/linux.sh install
./install/linux.sh update
./install/macos.sh test
./install/termux.sh uninstall
```

```powershell
powershell -ExecutionPolicy Bypass -File .\install\windows.ps1 install
.\install\windows.cmd update
```
