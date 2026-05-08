# Security Policy

## Scope

This policy covers security issues in the LowHunt tool itself.

It does not cover:

- findings discovered on targets scanned with LowHunt
- misuse of LowHunt against unauthorized targets
- third-party service outages, blocking, bans, policy changes, or schema drift
- local environment or dependency issues outside the LowHunt codebase

## Supported Versions

Security fixes are currently focused on:

- the latest released version
- the current main development branch

Older versions may not receive active security fixes.

## Responsible Use

LowHunt is intended for:

- educational use
- public-data-only OSINT
- defensive investigations
- authorized security research

Users are responsible for ensuring they have proper authorization before running the tool against any real target.

Unauthorized use, abuse, or illegal activity is not supported by this project.

## Reporting a Vulnerability

Do not open a public GitHub issue for a security vulnerability in LowHunt itself.

Report vulnerabilities privately to:

- `voltsparx@gmail.com`

Please include:

- affected version
- operating system and toolchain details
- a short impact summary
- clear reproduction steps
- proof-of-concept details if safe to share
- any suggested mitigation or workaround you found

## Response Expectations

Target expectations:

- acknowledgment: within 7 days
- follow-up status: within 30 days when the report is valid and reproducible

Response time can vary depending on severity, reproducibility, and maintainer availability.

## Safe Handling

- Share only the minimum information needed to reproduce the issue.
- Do not include credentials, private keys, tokens, or unrelated sensitive data.
- If the issue involves live infrastructure or third-party services, reproduce it only in an environment you own or are explicitly authorized to test.

## Disclosure

Please wait for confirmation before public disclosure.

When possible, a fix, mitigation, or risk note should be prepared before details are disclosed publicly.

## Out of Scope

The following are generally out of scope for this security policy:

- false positives or false negatives in third-party public sources
- target-side exposure discovered through intended passive use of the tool
- rate-limiting behavior from external services
- dependency installation failures in local environments

## Legal Notice

LowHunt is provided as-is without warranty.

The maintainers are not responsible for misuse of the software. Users are solely responsible for lawful and ethical operation.
