# Installers

## Purpose

LowHunt ships managed local installers under `install/`.

Supported installer scripts:

- `install/linux.sh`
- `install/macos.sh`
- `install/termux.sh`
- `install/windows.ps1`
- `install/windows.cmd`

## Supported Modes

- `install`
- `update`
- `uninstall`
- `test`

## Behavior

The installers:

- stage a new install before promotion
- create a managed install manifest
- create launchers in a user-local bin path
- run basic smoke checks
- avoid overwriting unmanaged directories

## Examples

Linux:

```sh
./install/linux.sh install
./install/linux.sh update
./install/linux.sh uninstall
./install/linux.sh test
```

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\install\windows.ps1 install
.\install\windows.cmd update
```

## Notes

- shell installers also manage PATH and MANPATH helper blocks
- Windows installers update the user PATH for the launcher directory
- `test-root/` is generated installer output, not source
