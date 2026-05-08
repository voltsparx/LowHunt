# Contributing to LowHunt

## Purpose

Thanks for contributing. LowHunt is meant to be a fast, public-data-only OSINT tool for authorized investigations, and good contributions help keep it useful, safe, and maintainable.

## Before You Start

Please read:

- `README.md`
- `docs/`
- `SECURITY.md`
- `CODE_OF_CONDUCT.md`

## What Good Contributions Look Like

Strong contributions usually improve one or more of these areas:

- scan accuracy
- passive source quality
- contact/email noise reduction
- engine reliability and fallback behavior
- output clarity and reporting
- documentation
- portability and installer quality
- tests and validation coverage

## What Not to Contribute

Please do not submit contributions that:

- bypass authentication or protections
- add credential attacks
- add intrusive or unauthorized active behavior outside project scope
- weaken attribution, provenance, or safety boundaries
- make the tool noisier without a strong investigation benefit

## Development Workflow

1. Read the relevant code and docs first.
2. Keep changes focused and scoped.
3. Prefer improving existing architecture rather than duplicating behavior.
4. Update docs when behavior changes.
5. Run validation before submitting.

## Repository Structure

Main source areas:

- `src/app/`: CLI entrypoint
- `src/config/`: config and platform loading
- `src/core/`: metadata and runtime helpers
- `src/engines/`: execution engines
- `src/net/`: HTTP, scanner, harvester, passive sources
- `src/modules/`: correlation and higher-level logic
- `src/output/`: direct outputs and report bundles
- `src/ui/`: banner and help output
- `docs/`: project documentation
- `man/`: manual page

## Style Expectations

For code:

- keep changes readable and explicit
- preserve public-data-only behavior
- prefer small, composable functions over tangled control flow
- maintain clear boundaries between orchestration, collection, execution, and reporting
- avoid unnecessary complexity or speculative abstraction

For docs:

- write for both beginners and experienced operators
- keep scope boundaries and safety language explicit
- update examples when CLI behavior changes

## Testing and Validation

Run the local validation flow when possible:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/run_all.ps1
```

Installer syntax checks:

```sh
bash -n install/linux.sh
bash -n install/macos.sh
bash -n install/termux.sh
```

Windows installer parse check:

```powershell
$text = Get-Content -LiteralPath 'install/windows.ps1' -Raw
[void][scriptblock]::Create($text)
```

If you are working in WSL or Linux and have the required libcurl headers installed, also run:

```sh
make clean
make
```

## Pull Request Guidance

Good pull requests usually include:

- a short explanation of what changed
- why the change matters
- any user-facing behavior changes
- docs updates if relevant
- test or validation notes

## Issues and Feature Requests

Useful issue reports generally include:

- expected behavior
- actual behavior
- reproduction steps
- platform and environment details
- logs or screenshots when helpful

Feature requests are more actionable when they explain:

- the investigator problem being solved
- why the current workflow is not enough
- how the proposed change fits the project scope

## Security Reports

If you believe you found a security vulnerability in LowHunt itself, do not open a public issue. Follow the private reporting instructions in `SECURITY.md`.

## Contact

Maintainer contact:

- `voltsparx@gmail.com`
