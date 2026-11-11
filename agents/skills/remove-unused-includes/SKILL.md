---
name: remove-unused-includes
description:
  'Find and safely remove unused #includes from Brave .cc files with
  clang-include-cleaner, keeping includes needed on other platforms and
  verifying the result with a build. Triggers on: remove unused includes, clean
  up includes, include-cleaner.'
argument-hint: '[folder ...]'
---

# Remove Unused Includes

Ported from Chromium's `//agents/skills/remove-unused-includes`. The upstream
script analyses and compiles one file at a time, which does not scale to the
Brave tree, and it trusts clang-include-cleaner more than is safe for Brave.
`remove_unused_includes.py` in this directory runs the same tool in resumable
stages:

1. **analyze**: runs clang-include-cleaner on every eligible `.cc` file in
   parallel and writes a plan.
2. **apply**: deletes the planned includes.
3. **verify**: builds, puts back what breaks, and repeats until the build is
   clean.

## Why the extra safeguards

- **Other platforms.** clang-include-cleaner only sees the current build
  configuration. An include used only inside `#if BUILDFLAG(IS_ANDROID)` code
  looks unused on Linux. `analyze` preprocesses each file with conditional
  blocks, finds the branches that are inactive, and keeps any removal whose
  header declares a name used there.
- **Forward declarations.** clang-include-cleaner treats a forward declaration
  in the matching `.h` as enough, so many removals fail with "incomplete type"
  errors. `verify` puts back only the includes whose headers declare a name
  quoted in that file's errors, or the whole file if none match.
- **Coverage.** An edited file whose objects the build did not recompile is
  reverted, as nothing checked it.
- **Scope.** Paths under `third_party`, `vendor` and `chromium_src` are never
  touched. `chromium_src` files are included into upstream files rather than
  compiled on their own.

## Steps

The script lives at `agents/skills/remove-unused-includes/` under `src/brave`.
Run it from `src/brave`. All state (database, plan, logs, backups) lives in
`--work-dir`, which defaults to `$TMPDIR/brave_remove_unused_includes`. Use a
fresh work directory for each run.

### 1. Analyse

```bash
python3 agents/skills/remove-unused-includes/remove_unused_includes.py analyze \
  [--folder components/foo ...] [--branch [--base origin/master]] \
  [--config Component]
```

`--branch` limits the analysis to files changed since the merge-base with
`--base` (default `origin/master`), including uncommitted changes. Without
`--folder`, the whole Brave tree is analysed. That takes roughly 15 minutes for
about 4000 files on a large machine. The first run generates the compilation
database from `out/<config>`. Pass `--regenerate-compdb` if the build directory
has changed since then. Files that were already analysed are cached, so an
interrupted run can be restarted.

Report the summary, and spot-check a few `inactive code` keeps in `plan.json`.

### 2. Apply

```bash
python3 agents/skills/remove-unused-includes/remove_unused_includes.py apply
```

Each edited file is backed up first. Emptied `#if` blocks and leftover blank
lines are tidied. A file whose planned `#include` lines are no longer there is
skipped.

### 3. Verify

```bash
python3 agents/skills/remove-unused-includes/remove_unused_includes.py verify \
  --build-arg=--gn=<arg>:<value> ...
```

This runs `pnpm run build <config> --ignore_compile_failure --target=brave:all`
until the build is clean. Pass the same `--gn` flags the build directory was
generated with, or the build regenerates it with different arguments. Logs are
saved as `build<N>.log` in the work directory. If a failure is outside the
edited files, the script stops. Fix it and rerun `verify`.

### 4. Review

```bash
git diff --stat
python3 agents/skills/remove-unused-includes/remove_unused_includes.py report
```

`restore` reverts every edited file from its backup.

## Other platforms

A Linux build cannot verify Windows, macOS, Android or iOS code. Before sending
the change for review, apply it (`git diff > includes.patch`) in a checkout for
each other platform and build there. `verify` only reverts files it edited
itself, so put back the includes that break there by hand. A separate
`analyze`/`apply`/`verify` run on that platform covers files that Linux does not
compile.
