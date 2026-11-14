---
name: brave/no-hand-edits-to-generated-test-filters
description:
  The files in test/filters/generated/ are refreshed with
  update-upstream-flake-filters.py and never edited by hand; manual entries go
  in the hand-written filter files.
rubric: testing
severity: should_fix
---

**Rule.** NEVER edit a `test/filters/generated/<suite>-<config>.filter` file by
hand: add, remove or change its entries only by rerunning
`vpython3 tools/chromium_tests_analysis/update-upstream-flake-filters.py` (for
some suites, or with other `--days` and `--min-flake-rate` values, as needed). A
flaky upstream test the script misses, as it can because LUCI Analysis only
exposes the top 200 failure clusters, is filtered by hand in the hand-written
files of `test/filters/`, following the rules for those.

**Why.** The script produces these files from per-config flake rates of the
matching upstream bots, and the next refresh overwrites any hand edit, silently
dropping or reviving entries.

## Examples

Bad:

```
# test/filters/generated/browser_tests-linux.filter, edited by hand
-SomeSuite.RareFlake
```

Good:

```bash
# in src/brave/
vpython3 tools/chromium_tests_analysis/update-upstream-flake-filters.py \
    browser_tests
```
