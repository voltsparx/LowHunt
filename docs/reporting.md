# Reporting

## Output Strategy

LowHunt supports both direct output files and stored report bundles.

Direct output is useful when you need one immediate artifact. Stored bundles are useful when you want a reusable case directory.

## Direct Output Formats

- `txt`
- `json`
- `csv`

## Stored Report Bundles

Default location:

```text
~/.lowhunt/output/reports/<target>/<timestamp>/
```

Bundle contents:

- `report.cli.txt`
- `report.txt`
- `report.json`

## Report Types

### Scan Bundle

Contains:

- profile results
- preset and engine metadata
- investigator summary context

### Harvest Bundle

Contains:

- passive host findings
- source overlap data
- contact emails
- confidence information

### Investigation Bundle

Contains:

- scan side summary
- harvest side summary
- correlation score
- narrative explanation

## Best Practices

- use `json` when feeding automation
- use `txt` for case notes and review
- keep stored bundles even when direct output is enabled
- use combined investigation mode when you need one shareable case result
