# LowHunt

LowHunt is a native C OSINT reconnaissance tool for authorized username and
domain footprint checks. It uses one JSON manifest per platform under
`platforms/`, libcurl-based HTTP probing, optional proxy/Tor routing, and
text/JSON/CSV output.

## Build

```sh
sudo apt install gcc make libcurl4-openssl-dev
make
```

## Usage

```sh
./lowhunt alice --fast
./lowhunt -u alice -o results.json --format json
./lowhunt -d example.com -b crtsh
./lowhunt --list-sites
```

## Platform Data

Platform metadata lives in `platforms/*.json`. Regenerate it from the reference
trees in `temp/` with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tmp/generate_platforms.ps1
```

Run the local checks with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/run_all.ps1
```

Only scan targets you own or have written authorization to assess.
