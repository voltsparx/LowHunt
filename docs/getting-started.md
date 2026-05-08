# Getting Started

## Purpose

This guide gets you from clone to first useful run.

## Build

Linux:

```sh
sudo apt install gcc make libcurl4-openssl-dev
make
```

## Install

Use the managed installer for your platform.

Linux:

```sh
./install/linux.sh install
```

macOS:

```sh
./install/macos.sh install
```

Termux:

```sh
./install/termux.sh install
```

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\install\windows.ps1 install
```

## First Commands

Check the help surface:

```sh
lowhunt --help
lowhunt --about
lowhunt --list-sites
lowhunt --list-sources
```

Run a beginner-friendly username scan:

```sh
lowhunt alice --preset beginner
```

Run a passive domain harvest:

```sh
lowhunt -d example.com -b all
```

Run a combined investigation:

```sh
lowhunt alice bob -d example.com -b all --intel
```

## Recommended First Workflow

1. Start with `--preset beginner`.
2. Use `--list-sources` to understand harvest options.
3. Use `-vv` only when you need full row visibility.
4. Save important runs with `-o` and `--format json`.
5. Review stored bundles in `~/.lowhunt/output/reports/`.

## When To Use Each Mode

- Username scan: when you only need profile footprinting.
- Harvest mode: when you only need public host and contact discovery.
- Combined investigation: when you want one case workflow and correlation.
