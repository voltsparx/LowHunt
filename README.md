# LowHunt

LowHunt is a native C OSINT reconnaissance tool for authorized username and
domain footprint checks. It uses a runtime JSON site database, libcurl-based
HTTP probing, optional proxy/Tor routing, and text/JSON/CSV output.

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

Only scan targets you own or have written authorization to assess.
