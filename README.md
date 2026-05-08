# LowHunt

LowHunt is a native C OSINT tool for authorized username and domain footprint checks against publicly available data. It uses one JSON manifest per platform under `platforms/`, libcurl-based HTTP probing, passive hostname harvesting, multiple execution engines, beginner-friendly presets, confidence-aware passive source fusion, and investigator-friendly terminal summaries.

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
./lowhunt alice --preset beginner
./lowhunt alice --engine fusion -vv
./lowhunt -u alice -o results.json --format json
./lowhunt -d example.com -b all
./lowhunt --list-sources
./lowhunt --about
./lowhunt --explain engine
./lowhunt --list-sites
```

Presets:

- `beginner`: steadier defaults, investigator summary enabled, easier-to-read output
- `balanced`: default behavior
- `aggressive`: fuller output and faster engine bias for large runs

Passive harvest notes:

- `-b all` fuses the currently wired public passive sources: `crtsh`, `rapiddns`, and `wayback`
- overlapping hostnames gain higher confidence scores
- same-domain contact emails are collected from public contact-oriented pages with built-in noise filtering

Report bundles:

- every scan and harvest now stores a small offline bundle under `~/.lowhunt/output/reports/<target>/<timestamp>/`
- bundle artifacts include `report.cli.txt`, `report.txt`, and `report.json`

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
