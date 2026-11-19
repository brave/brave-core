---
name: brave/comment-filter-file-entries
description:
  Every disabled test entry in a filter file is preceded by a comment saying why
  it is disabled, what condition fails, why this file, and the upstream
  flakiness data.
rubric: readability
severity: should_fix
---

**Rule.** Every disabled test entry (`-Suite.Test`) added to a filter file MUST
be preceded by a comment that states:

1. why the test is disabled;
2. what specific condition causes the failure;
3. why this filter file was chosen, unless the file name makes it obvious;
4. for an upstream Chromium test, the upstream flakiness data: the
   `is_upstream_flake.py` status, the builders it applies to and the branch
   checked.

A bare entry, or a comment that only says the test is flaky, does not meet this.

**Why.** Without this context nobody can tell later whether the entry is still
needed, whether it hides a Brave bug, or when it can be removed.

## Examples

Bad:

```
-WebUIURLLoaderFactoryTest.RangeRequest/*
```

Good:

```
# Confirmed upstream flake on refs/branch-heads/8103 per LUCI Analysis
# (FLAKY on linux-rel and Linux Tests).
# Mojo data pipe race condition — completion signal arrives before data is
# flushed through the consumer side.
-WebUIURLLoaderFactoryTest.RangeRequest/*
```
