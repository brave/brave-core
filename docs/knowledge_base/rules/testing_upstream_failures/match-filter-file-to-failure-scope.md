---
name: brave/match-filter-file-to-failure-scope
description:
  A filter entry goes in the narrowest filter file (platform, sanitizer) the
  upstream evidence supports, and in a broader file when the test also fails
  beyond that platform or sanitizer.
rubric: testing
severity: should_fix
---

**Rule.** Put each filter entry in the most specific filter file the evidence
supports, and no more specific. When a test only fails on one platform or under
one sanitizer, and `is_upstream_flake.py` (which reports each builder
separately) shows no failures elsewhere, use the platform or
platform-and-sanitizer file (such as `browser_tests-linux-asan.filter`), not an
all-platform one. Do NOT put the entry in a sanitizer-specific file (`-asan`,
`-msan`) when non-sanitizer builders are among those FLAKY or FAILING: use a
broader file (such as `browser_tests.filter` or `browser_tests-win.filter`) that
covers every environment where the failure may occur.

**Why.** A file broader than the failure disables the test where it passes and
loses that coverage; a file narrower than the failure leaves the test failing on
the other builds.

## Examples

Bad:

```
# test/filters/browser_tests-linux-asan.filter
# FLAKY on linux-rel and Linux ASan Tests upstream.
-SomeSuite.SomeTest
```

Good:

```
# test/filters/browser_tests-linux.filter
# FLAKY on linux-rel and Linux ASan Tests upstream.
-SomeSuite.SomeTest
```
