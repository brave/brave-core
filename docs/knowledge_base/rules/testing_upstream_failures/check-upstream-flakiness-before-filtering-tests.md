---
name: brave/check-upstream-flakiness-before-filtering-tests
description:
  An upstream test is added to a filter file only once is_upstream_flake.py
  shows it FLAKY or FAILING upstream; a STABLE result means Brave's overrides
  and patches are investigated, and a Brave-caused failure is fixed, not
  filtered.
rubric: testing
severity: blocking
---

**Rule.** Before an upstream Chromium test is added to a filter file, Brave code
MUST be ruled out as the cause of its failure. Check it with
`vpython3 tools/chromium_tests_analysis/is_upstream_flake.py "Suite.Test"`
(`--os` for the platform that failed, `--ref main` against upstream main): a
test FLAKY or FAILING on the platform that failed may be filtered. For a STABLE
test, investigate Brave-specific causes first: the overrides in `chromium_src/`
for the test's directory tree and any patch in `patches/` touching the same
files; NO DATA needs manual investigation. When a Brave override or patch makes
the test fail, fix that code; do NOT hide the failure with a filter.

**Why.** A filter on a test that only fails because of Brave hides a Brave bug,
and the coverage the test gives is lost for as long as the entry stays.

## Examples

Bad:

```
# Fails on our bots.
-SomeSuite.SomeTest
```

Good:

```
# Confirmed upstream flake on refs/branch-heads/8103 per LUCI Analysis
# (FLAKY on linux-rel and Linux Tests).
# Mojo data pipe race condition — completion signal arrives before data is
# flushed through the consumer side.
-WebUIURLLoaderFactoryTest.RangeRequest/*
```
