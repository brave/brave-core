---
name: brave/no-log-error-for-non-critical-failures
description:
  'LOG(ERROR) is kept for unexpected, serious failures; known non-critical
  failures log with VLOG, LOG(WARNING) or DLOG(ERROR), and a reviewer suggests
  lowering severity only once a failure is confirmed non-critical.'
rubric: readability
severity: should_fix
---

**Rule.** Use `LOG(ERROR)` only for unexpected, serious failures. For an
expected or non-critical failure (a bad user-supplied filter list, a failed
parse of optional data), use `VLOG` for debugging information, `LOG(WARNING)`
for a noteworthy but non-critical issue, or `DLOG(ERROR)` for an error that
matters only in debug builds. NEVER suggest lowering the severity of a log
unless the failure is clearly non-critical: the author knows better than the
code shows whether, for example, a model file failing to load breaks the service
that depends on it.

**Why.** `LOG(ERROR)` lines fill release logs and users' consoles with
non-actionable noise that buries the errors that matter.

## Examples

Bad:

```cpp
LOG(ERROR) << "Failed to parse filter list";
```

Good:

```cpp
VLOG(1) << "Failed to parse filter list";
```
