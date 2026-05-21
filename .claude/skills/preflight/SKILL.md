---
name: preflight
description:
  'Run all preflight checks (format, gn_check, presubmit, build, tests) to make
  sure the current work is ready for review.'
disable-model-invocation: true
---

# Preflight Checks

Run all preflight checks to make sure the current work is ready for review.
Execute each step sequentially and stop immediately if any step fails.

## Arguments

- **`all`** — Run all test suites (brave_browser_tests, brave_unit_tests,
  brave_component_unittests) without filters, instead of only running tests
  affected by the change. Usage: `/preflight all`

## Current State

- Branch: !`git branch --show-current`
- Status: !`git status --short`

## Steps

### 0. Check branch

If on `master`, create a new branch off of master before proceeding (use a
descriptive branch name based on the changes).

### 1. Format code

Run `npm run format`. If formatting changes any files, stage and include them in
the commit later.

### 2. GN check

Run `npm run gn_check`. Fix any issues found and re-run until it passes.

**Skip this step** if the only changes are to test filter files
(`test/filters/*.filter`) — filter files don't affect GN build configuration.

### 3. Presubmit

Run `npm run presubmit`. Fix any issues found and re-run until it passes.

### 4. Commit if needed

Check `git status`. If there are any uncommitted changes (staged, unstaged, or
untracked files relevant to the work), create a commit. The commit message
should be short and succinct, describing what was done. If there are no changes,
skip this step.

### 5. Build

Run `npm run build` to make sure the code builds. If it fails, fix the build
errors, amend the commit, and retry.

If work is targeting a different platform, follow the build instructions at
README.md or confirm with the user which build arguments or technique to use.

### 6. Run tests

**If the `all` argument was provided:** Run all test suites without filters:

- `npm run test -- brave_browser_tests`
- `npm run test -- brave_unit_tests`
- `npm run test -- brave_component_unittests`
- `npm run test-unit`

**Otherwise (default):** Determine which test suites are affected by the changes
in this branch (compare against `master`). Look at the changed files and
identify the corresponding test suites and relevant test filters.

- `npm run test -- [test suite] --filter="..."`

where test suite is any of:

- brave_components_unittests (C++ unit tests in brave/components)
- brave_unit_tests (C++ unit tests anywhere else in brave/)
- brave_browser_tests
- chromium_unit_tests (Chromium C++ unit tests in src/)
- browser_tests (Chromium browser tests in src/)

Or `npm run test-unit -- [path blob filter]` to run `jest` for any
\*.test.{ts,tsx} files.

If no tests are affected, note that and move on.

### 7. Re-run checks if fixes were needed

If any step required fixes (build errors, test failures, format/lint issues),
amend the commit with the fixes and re-run checks until everything passes
cleanly.

## Important

- Stop and report if any step fails after exhausting reasonable fix attempts.
- Report a summary of results when all steps complete successfully.
