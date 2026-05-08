# Platform Manifests

## Purpose

Platform manifests define how username scanning works for each site.

They live in:

```text
platforms/*.json
```

## What A Manifest Represents

A manifest tells LowHunt:

- the platform name
- the URL template
- the detection strategy
- not-found indicators
- category information
- NSFW classification

## Typical Fields

- `name`
- `url`
- `url_template`
- `errorMsg`
- `errorUrl`
- `errorCode`
- `errorType`
- `category`
- `isNSFW`

## Detection Methods

LowHunt currently supports:

- status-code based detection
- string-match based detection
- response-URL based detection

## Operational Advice

- keep manifests narrow and explicit
- avoid ambiguous false-positive logic
- prefer stable public paths
- validate new manifests before trusting them in real investigations

## Regeneration

If you use the local helper flow:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tmp/generate_platforms.ps1
```
