---
name: brave/blank-line-before-filter-comment-sections
description:
  In a filter file, a new comment section is always preceded by a blank line,
  never placed right after a test entry.
rubric: readability
severity: nit
---

**Rule.** In filter files, ALWAYS leave a blank line before a new comment
section. Do not put a comment line directly after a test entry.

**Why.** The blank line shows where one root-cause section ends and the next
begins; without it a comment reads as if it described the entries above it.

## Examples

Bad:

```
# Mojo pipe race condition — data flushed after completion signal.
-SuiteA.Test1
# Upstream flake: timing-dependent resource load on slow bots (2.3% / 30d).
-SuiteB.Test3
```

Good:

```
# Mojo pipe race condition — data flushed after completion signal.
-SuiteA.Test1

# Upstream flake: timing-dependent resource load on slow bots (2.3% / 30d).
-SuiteB.Test3
```
