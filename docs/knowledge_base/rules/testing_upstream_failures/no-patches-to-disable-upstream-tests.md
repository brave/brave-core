---
name: brave/no-patches-to-disable-upstream-tests
description:
  An upstream Chromium test failing intermittently is disabled with an entry in
  a test/filters/ file, never by a patch that renames it DISABLED_, adds a
  MAYBE_ or skips it.
rubric: testing
severity: blocking
---

**Rule.** NEVER disable an upstream Chromium test for an intermittent failure by
patching its source in `patches/`: no `DISABLED_` or `MAYBE_` prefixes, no
`GTEST_SKIP()`, no `@DisabledTest` annotations or the like added by a patch.
Disable it with a documented entry in the appropriate filter file in
`test/filters/` instead.

**Why.** Filter files are the preferred way to disable flaky upstream tests: an
entry is trivially reversible and costs nothing on a rebase, whereas a patch to
an upstream test file has to be maintained and conflicts as upstream changes the
test.

## Examples

Bad:

```diff
-IN_PROC_BROWSER_TEST_F(WebUIURLLoaderFactoryTest, RangeRequest) {
+IN_PROC_BROWSER_TEST_F(WebUIURLLoaderFactoryTest, DISABLED_RangeRequest) {
```

Good:

```
# Confirmed upstream flake on refs/branch-heads/8103 per LUCI Analysis
# (FLAKY on linux-rel and Linux Tests).
# Mojo data pipe race condition — completion signal arrives before data is
# flushed through the consumer side.
-WebUIURLLoaderFactoryTest.RangeRequest/*
```
