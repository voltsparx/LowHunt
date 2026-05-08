# CLI Reference

## Main Forms

```sh
lowhunt [OPTIONS] <username> [username2 ...]
lowhunt -d <domain> [OPTIONS]
lowhunt <username> -d <domain> [OPTIONS]
```

## Core Options

### Targets

- `-u, --username <name>`: add a username target
- `-d, --domain <domain>`: run passive domain harvest mode
- `-s, --site <name>`: limit a scan to one platform

### Harvest Control

- `-b, --source <name>`: select harvest source
- `-l, --limit <n>`: limit displayed harvested hosts
- `--list-sources`: show wired passive harvest sources

### Execution

- `--preset <name>`: `beginner`, `balanced`, `aggressive`
- `--engine <name>`: `auto`, `threadpool`, `parallel`, `async`, `fusion`, `stabilizer`, `sync`
- `-T, --threads <n>`: requested concurrency
- `-t, --timeout <ms>`: request timeout

### Network Posture

- `--tor`: route through `socks5://127.0.0.1:9050`
- `--proxy <url>`: use a custom proxy

### Output

- `-v`: verbose output
- `-vv`: full verbose structured output
- `-o, --output <file>`: write direct output to file
- `--format <fmt>`: `txt`, `json`, `csv`
- `--intel`: show additional investigator summary

### Catalog and Help

- `--list-sites`: list supported platforms
- `--about`: show banner and project details
- `--explain <topic>`: explain a command or concept
- `--version`: print version
- `-h, --help`: show help

## Explain Topics

Current explain topics include:

- `scan`
- `investigate`
- `preset`
- `harvest`
- `sources`
- `engine`
- `fusion`
- `stabilizer`
- `threads`
- `-v`
- `-vv`
- `about`
- `reports`
- `manual`

## Example Commands

```sh
lowhunt alice --preset beginner
lowhunt alice --engine fusion -vv
lowhunt -d example.com -b all
lowhunt alice bob -d example.com -b all --intel
lowhunt --explain reports
```
