# Engines

## Purpose

LowHunt uses execution engines to decide how work is run. Engines do not define the investigation logic itself. They execute the work plan.

## Available Engines

### `auto`

Chooses an engine based on:

- workload size
- thread count
- proxy or Tor use
- general runtime posture

### `threadpool`

Balanced and dependable.

Best for:

- everyday scans
- moderate workloads
- stable default use

### `parallel`

Simple parallel execution.

Best for:

- moderate task fanout
- straightforward batch work

### `async`

Useful for larger I/O-heavy runs.

Best for:

- bigger scans
- many concurrent HTTP checks

### `fusion`

Higher-throughput bias.

Best for:

- aggressive large runs
- strong local resources

### `stabilizer`

Lower-pressure pacing.

Best for:

- Tor
- proxies
- rate-sensitive environments

### `sync`

Sequential and simple.

Best for:

- troubleshooting
- tiny runs
- baseline comparison

## Fault Isolation

On supported platforms, engine execution can be isolated so a bad engine failure does not destroy the whole scan. If the selected engine fails early or exits badly, LowHunt can fall back and continue remaining work with another engine.

## Resource Guard

If threads are set above `50`, LowHunt performs a basic local resource check and warns when the machine appears constrained.

## Recommended Usage

- `beginner` preset: usually safest with `stabilizer`-style behavior
- normal daily work: `auto` or `threadpool`
- large scans: `fusion` or `async`
- sensitive paths: `stabilizer`
