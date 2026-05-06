# LowHunt Installers

Managed local installers live here.

Available scripts:

- `linux.sh`
- `macos.sh`
- `termux.sh`
- `windows.ps1`
- `windows.cmd`

Supported modes:

- `install`
- `update`
- `uninstall`
- `test`

Examples:

```sh
./install/linux.sh
./install/linux.sh update
./install/macos.sh test
./install/termux.sh uninstall
```

```powershell
powershell -ExecutionPolicy Bypass -File .\install\windows.ps1 install
```
