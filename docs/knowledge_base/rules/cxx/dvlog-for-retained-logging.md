---
name: brave/dvlog-for-retained-logging
description:
  'Logging added for debugging is removed before check-in; logging that must
  stay has a specific purpose and uses DVLOG(1).'
rubric: perf
severity: should_fix
---

**Rule.** Remove logging added while developing before the change is checked in.
A log statement that remains must have a specific purpose, such as investigating
a rare bug. When logging must remain, prefer `DVLOG(1)` to `LOG`, `VLOG` and the
other macros.

**Why.** Stray logs bloat the release binary with their strings and fill logs
with noise. `DVLOG(1)` is compiled out of release builds, and can be turned on
with `--v=1` or `--vmodule=mod=1` when needed.

## Examples

Bad:

```cpp
LOG(INFO) << "Processing state: " << state;
```

Good:

```cpp
DVLOG(1) << "Processing state: " << state;
```
