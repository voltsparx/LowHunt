# Architecture

## High-Level Design

LowHunt is built around a simple but strict separation of responsibilities:

- configuration decides what to do
- engines decide how work executes
- harvest sources collect passive public intelligence
- scanners classify profile results
- modules interpret and correlate findings
- output layers render and store artifacts

## Main Runtime Stages

1. CLI parsing
2. preset application
3. engine planning
4. resource guard check
5. scan and or harvest execution
6. result classification
7. correlation and summaries
8. direct outputs and stored bundles

## Important Boundaries

### Engines Are Executors

Engines should execute planned tasks. They should not silently become their own orchestration policy layer.

### Passive Sources Are Collectors

Passive sources should focus on collecting and normalizing public intelligence. They should not own reporting or global execution strategy.

### Correlation Is Interpretation

Correlation logic should consume findings and produce higher-level signals. It should not mutate the raw collection rules.

### Reporting Is Its Own Layer

Direct outputs and stored bundles should stay separate from collection logic so the same findings can be rendered in more than one way.

## Resilience Design

LowHunt includes:

- engine fallback behavior
- resource warnings for high thread counts
- passive-source deduplication
- built-in contact noise filtering
- combined investigation reporting instead of ad hoc terminal-only state

## Main Source Areas

- `src/app/`: CLI entrypoint
- `src/config/`: site and config loading
- `src/core/`: metadata and runtime helpers
- `src/engines/`: execution engines
- `src/net/`: HTTP, scanner, harvester, passive sources
- `src/modules/`: correlation and higher-level analysis
- `src/output/`: output files and report bundles
- `src/ui/`: banner and help surfaces
