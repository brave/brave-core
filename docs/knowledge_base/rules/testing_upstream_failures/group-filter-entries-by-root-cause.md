---
name: brave/group-filter-entries-by-root-cause
description:
  In a filter file, disabled tests sharing a root cause are grouped under one
  comment section, and tests with a distinct root cause get their own section.
rubric: readability
severity: should_fix
---

**Rule.** In filter files, list the disabled tests that share a root cause
together under a single comment section describing that cause. A test with a
different root cause gets its own section with its own comment; do not mix
causes under one vague comment such as "Various flaky tests".

**Why.** One section per root cause lets a reader see what each comment covers,
and lets the whole group be re-enabled together once its cause is fixed.

## Examples

Bad:

```
# Various flaky tests
-SuiteA.Test1
-SuiteB.Test2
```

Good:

```
# Mojo pipe race condition — data flushed after completion signal.
-SuiteA.Test1
-SuiteA.Test2

# Upstream flake: timing-dependent resource load on slow bots (2.3% / 30d).
-SuiteB.Test3
```
