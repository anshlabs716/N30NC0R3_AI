# Development

This page describes how to set up a development environment and work on
neoncore.

## Requirements

- A Linux system (Ubuntu/Debian style)
- `build-essential`, `make`, `pkg-config`, `cmake`, `git`
- `libgtk-3-dev`, `libcurl4-openssl-dev`, `libespeak-ng-dev`

## Getting the code

```bash
git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI
```

## Build

```bash
make
```

## Run

```bash
./neoncore
```

## Test

```bash
# Manual testing (see docs/testing.md)
```

## Workflow

- Branch from `main` (see [CONTRIBUTING.md](../CONTRIBUTING.md)).
- Keep changes focused and documented.
- Update `CHANGELOG.md` for user-facing changes.
