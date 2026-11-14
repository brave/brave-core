---
name: brave/disable-upstream-flakes-rather-than-patch-fixes
description:
  An intermittent failure of an upstream Chromium test is filtered rather than
  fixed by modifying the upstream test code, unless Brave's own chromium_src
  overrides or patches cause it.
rubric: testing
severity: should_fix
---

**Rule.** Do not modify upstream Chromium test code, through a patch or a
`chromium_src` override of the test, to fix an intermittent failure of an
upstream test. Prefer disabling it with a well-documented filter file entry,
which is the correct long-term solution for an upstream flake, in particular
when `is_upstream_flake.py` reports it FLAKY or FAILING on the platform that
failed. Change the upstream test code only when Brave's own `chromium_src`
overrides or patches are responsible for the failure.

**Why.** Modifying upstream test code via patches is fragile and
maintenance-heavy: every rebase has to carry the change, and it conflicts
whenever upstream touches the test. A filter entry costs nothing to keep and is
removed when upstream fixes the flake.

## Examples

Bad:

```diff
 IN_PROC_BROWSER_TEST_F(WebUIURLLoaderFactoryTest, RangeRequest) {
+  // Work around the upstream data pipe race.
+  base::PlatformThread::Sleep(base::Milliseconds(50));
```

Good:

```
# Confirmed upstream flake on refs/branch-heads/8103 per LUCI Analysis
# (FLAKY on linux-rel and Linux Tests).
# Mojo data pipe race condition — completion signal arrives before data is
# flushed through the consumer side.
-WebUIURLLoaderFactoryTest.RangeRequest/*
```
