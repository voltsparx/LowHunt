# LowHunt

## 🔎 Fast Public OSINT for Real Investigations

LowHunt is a native C OSINT tool for authorized investigations on publicly available data. It helps investigators, defenders, analysts, and curious operators move from scattered checks to a structured workflow for username intelligence, passive domain collection, contact discovery, correlation, and reporting.

It is built for public-data-only use. It does not bypass authentication, protections, or access controls.

## ✨ Why Use LowHunt

LowHunt is designed for people who want:

- a fast native binary instead of a heavier runtime stack
- one workflow for username scans, passive domain harvests, and combined investigations
- engine-driven execution with fault isolation and fallback behavior
- quieter contact collection with noise reduction and built-in filtering
- operator-friendly output that still works for automation and reporting

## 🧩 What It Solves

OSINT work often gets split across too many disconnected steps:

- one tool for usernames
- another for passive domain intelligence
- another script for emails
- another notebook for summaries
- another format for reporting

LowHunt reduces that fragmentation. It gives you one CLI that can:

- scan usernames across a large platform set
- harvest passive hostname intelligence from public sources
- collect same-domain public contact emails with filtering
- correlate profile hits with harvested domain-side artifacts
- store reusable report bundles for later review

## 🚀 Core Capabilities

<table>
  <thead>
    <tr>
      <th>Capability</th>
      <th>What It Does</th>
      <th>Why It Matters</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Username scanning</td>
      <td>Checks platform manifests for public profile signals</td>
      <td>Quickly maps a username footprint across many sites</td>
    </tr>
    <tr>
      <td>Passive domain harvest</td>
      <td>Fuses public host intelligence from <code>crtsh</code>, <code>rapiddns</code>, and <code>wayback</code></td>
      <td>Builds a broader view of public domain exposure without active intrusion</td>
    </tr>
    <tr>
      <td>Contact discovery</td>
      <td>Collects same-domain public contact emails from contact-oriented pages</td>
      <td>Helps analysts find business-facing identifiers with less noise</td>
    </tr>
    <tr>
      <td>Correlation</td>
      <td>Compares profile hits with harvested domain-side email identifiers</td>
      <td>Turns disconnected findings into investigation signals</td>
    </tr>
    <tr>
      <td>Execution engines</td>
      <td>Supports <code>auto</code>, <code>threadpool</code>, <code>parallel</code>, <code>async</code>, <code>fusion</code>, <code>stabilizer</code>, and <code>sync</code></td>
      <td>Lets operators bias for speed, steadiness, or safer recovery</td>
    </tr>
    <tr>
      <td>Report bundles</td>
      <td>Stores CLI, TXT, and JSON artifacts per run</td>
      <td>Makes results easier to revisit, review, and hand off</td>
    </tr>
  </tbody>
</table>

## 🛠️ Why LowHunt Feels Different

<table>
  <thead>
    <tr>
      <th>Area</th>
      <th>LowHunt Approach</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Performance</td>
      <td>Native C runtime, multiple execution engines, concurrency-aware presets</td>
    </tr>
    <tr>
      <td>Usability</td>
      <td>Beginner-friendly presets, explain topics, readable summaries, strong defaults</td>
    </tr>
    <tr>
      <td>Resilience</td>
      <td>Engine isolation and fallback behavior so one engine failure does not kill the whole run</td>
    </tr>
    <tr>
      <td>Signal quality</td>
      <td>Built-in contact/email filtering, source overlap confidence, report-side narrative summaries</td>
    </tr>
    <tr>
      <td>Workflow coverage</td>
      <td>Username OSINT, passive domain intelligence, combined investigation mode, bundled reporting</td>
    </tr>
  </tbody>
</table>

## 🧭 How the Tool Works

```text
┌─────────────────────────────────────────────────────────────────────┐
│ 1. Operator Input                                                  │
│    usernames, domain, engine, preset, timeout, threads, outputs    │
└─────────────────────────────────────────────────────────────────────┘
                              |
                              v
┌─────────────────────────────────────────────────────────────────────┐
│ 2. Runtime Planning                                                │
│    preset application → engine selection → resource warning logic   │
└─────────────────────────────────────────────────────────────────────┘
                              |
                ┌─────────────┴─────────────┐
                v                           v
┌──────────────────────────────┐  ┌──────────────────────────────────┐
│ 3A. Username Scan Pipeline   │  │ 3B. Passive Harvest Pipeline     │
│ platform manifests loaded    │  │ public sources selected          │
│ scan tasks built             │  │ host intelligence fused          │
│ engine executes requests     │  │ contact pages collected          │
│ results classified           │  │ emails filtered and scored       │
└──────────────────────────────┘  └──────────────────────────────────┘
                |                           |
                └─────────────┬─────────────┘
                              v
┌─────────────────────────────────────────────────────────────────────┐
│ 4. Correlation Layer                                               │
│ profile hits + harvested hosts + same-domain emails → confidence    │
│ score, overlap analysis, investigation narrative                    │
└─────────────────────────────────────────────────────────────────────┘
                              |
                              v
┌─────────────────────────────────────────────────────────────────────┐
│ 5. Output Layer                                                    │
│ terminal output → file output → stored report bundles               │
│ report.cli.txt / report.txt / report.json                           │
└─────────────────────────────────────────────────────────────────────┘
```

## ⚡ Quick Start

### Build

```sh
sudo apt install gcc make libcurl4-openssl-dev
make
```

### Install

Use the managed local installers in `install/`.

```sh
./install/linux.sh install
./install/macos.sh install
./install/termux.sh install
```

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\install\windows.ps1 install
```

Or:

```cmd
install\windows.cmd install
```

## 🧪 Common Commands

```sh
./lowhunt alice --preset beginner
./lowhunt alice --engine fusion -vv
./lowhunt -u alice -o results.json --format json
./lowhunt -d example.com -b all
./lowhunt alice bob -d example.com -b all --intel
./lowhunt --list-sites
./lowhunt --list-sources
./lowhunt --about
./lowhunt --explain investigate
```

## 🎛️ Presets

<table>
  <thead>
    <tr>
      <th>Preset</th>
      <th>Behavior</th>
      <th>Best For</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>beginner</code></td>
      <td>Readable defaults, steadier pacing, intelligence summary enabled</td>
      <td>New users and careful operator review</td>
    </tr>
    <tr>
      <td><code>balanced</code></td>
      <td>Default behavior with no heavy bias</td>
      <td>Everyday OSINT work</td>
    </tr>
    <tr>
      <td><code>aggressive</code></td>
      <td>Higher concurrency floor, fuller visibility, fast-engine bias</td>
      <td>Large runs where the environment can handle it</td>
    </tr>
  </tbody>
</table>

## 🧠 Engines

<table>
  <thead>
    <tr>
      <th>Engine</th>
      <th>Role</th>
      <th>Best Use</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>auto</code></td>
      <td>Picks an engine from workload and network posture</td>
      <td>General use</td>
    </tr>
    <tr>
      <td><code>threadpool</code></td>
      <td>Balanced worker model</td>
      <td>Steady default scanning</td>
    </tr>
    <tr>
      <td><code>parallel</code></td>
      <td>Simple parallel execution</td>
      <td>Moderate workloads</td>
    </tr>
    <tr>
      <td><code>async</code></td>
      <td>Cooperative concurrency for larger I/O-heavy workloads</td>
      <td>Bigger scans</td>
    </tr>
    <tr>
      <td><code>fusion</code></td>
      <td>High-throughput execution bias</td>
      <td>Fast large runs</td>
    </tr>
    <tr>
      <td><code>stabilizer</code></td>
      <td>Lower-pressure pacing</td>
      <td>Tor, proxies, rate-sensitive work</td>
    </tr>
    <tr>
      <td><code>sync</code></td>
      <td>Sequential, simplest path</td>
      <td>Troubleshooting and tiny runs</td>
    </tr>
  </tbody>
</table>

## 🌐 Passive Harvesting

LowHunt currently supports:

<table>
  <thead>
    <tr>
      <th>Source</th>
      <th>Purpose</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>crtsh</code></td>
      <td>Certificate-transparency hostname discovery</td>
    </tr>
    <tr>
      <td><code>rapiddns</code></td>
      <td>Public passive subdomain listing</td>
    </tr>
    <tr>
      <td><code>wayback</code></td>
      <td>Archived URL hostname extraction</td>
    </tr>
    <tr>
      <td><code>all</code></td>
      <td>Fuses all wired public sources and increases confidence when hosts overlap</td>
    </tr>
  </tbody>
</table>

Noise reduction and filtering behavior:

- same-domain contact emails only
- `noreply`-style addresses suppressed
- asset-like strings filtered out
- malformed candidates ignored
- public contact-oriented pages prioritized

## 🔗 Combined Investigations

When usernames and `-d <domain>` are supplied together, LowHunt runs a combined case workflow:

- passive host intelligence is collected
- contact emails are harvested and filtered
- username scan results are generated
- the correlation layer compares usernames against harvested identifiers
- an investigation confidence score and narrative are printed
- report bundles are stored for later review

Example:

```sh
./lowhunt alice bob -d example.com -b all --intel -o investigation.json --format json
```

## 📦 Outputs and Report Bundles

Direct output formats:

- `txt`
- `json`
- `csv`

Stored report bundles:

```text
~/.lowhunt/output/reports/<target>/<timestamp>/
```

Bundle contents:

- `report.cli.txt`
- `report.txt`
- `report.json`

Combined investigations also store a fused investigation bundle with correlation narrative and confidence details.

## 🧯 Safety and Scope

LowHunt is intentionally scoped for public-data-only OSINT.

- no authentication bypass
- no credential attacks
- no protection circumvention
- no hidden active intrusion behavior

Use it only on:

- assets you own
- systems you operate
- targets you have explicit written authorization to assess

## 📚 Documentation

The full documentation set lives in [`docs/`](docs/index.md).

Suggested reading order:

1. [`docs/index.md`](docs/index.md)
2. [`docs/getting-started.md`](docs/getting-started.md)
3. [`docs/cli-reference.md`](docs/cli-reference.md)
4. [`docs/engines.md`](docs/engines.md)
5. [`docs/harvest-and-investigation.md`](docs/harvest-and-investigation.md)
6. [`docs/reporting.md`](docs/reporting.md)
7. [`docs/architecture.md`](docs/architecture.md)

## 🧾 Manual

After installation:

```sh
man lowhunt
```

Manual source:

- [`man/lowhunt.1`](man/lowhunt.1)

## 🧱 Platform Manifests

Platform metadata lives under `platforms/*.json`.

Regenerate platform data from the local reference trees with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tmp/generate_platforms.ps1
```

## ✅ Validation

Local validation:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/run_all.ps1
```

Installer parsing checks:

```sh
bash -n install/linux.sh
bash -n install/macos.sh
bash -n install/termux.sh
```

## 👤 Author

- Author: `voltsparx`
- Contact: `voltsparx@gmail.com`
- Repository: `https://github.com/voltsparx/LowHunt`
- License: `MIT`

## 🤝 Final Note

LowHunt is built to help public-data investigations feel faster, cleaner, and more structured. If you want a native tool that can move from first query to stored investigation bundle without turning into a maze of scripts, that is exactly the problem it is meant to solve.
