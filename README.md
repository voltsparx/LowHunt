# LowHunt

LowHunt is a native C OSINT tool for authorized username and domain footprint checks against publicly available data. It uses one JSON manifest per platform under `platforms/`, libcurl-based HTTP probing, passive hostname harvesting, multiple execution engines, and investigator-friendly terminal summaries.

## Build

```sh
sudo apt install gcc make libcurl4-openssl-dev
make
```

## Install

Use the managed local installers under `install/`:

```sh
./install/linux.sh
./install/macos.sh
./install/termux.sh
```

On Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\install\windows.ps1
```

Or:

```cmd
install\windows.cmd
```

## Usage

```sh
./lowhunt alice --engine fusion -vv
./lowhunt -u alice -o results.json --format json
./lowhunt -d example.com -b crtsh
./lowhunt --about
./lowhunt --explain engine
./lowhunt --list-sites
```

## Manual

The project ships a `man` page source in `man/lowhunt.1`. After a local install:

```sh
man lowhunt
```

## Platform Data

Platform metadata lives in `platforms/*.json`. Regenerate it from the reference trees in `temp/` with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tmp/generate_platforms.ps1
```

Run the local checks with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/run_all.ps1
```

Only scan targets you own or have written authorization to assess.
