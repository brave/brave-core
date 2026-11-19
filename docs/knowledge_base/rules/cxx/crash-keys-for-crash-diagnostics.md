---
name: brave/crash-keys-for-crash-diagnostics
description:
  'Data needed to diagnose crashes is attached with
  base::debug::ScopedCrashKeyString, not written to logs.'
rubric: design
severity: should_fix
---

**Rule.** When code records state to understand crashes, set a crash key with
`base::debug::ScopedCrashKeyString` (or `SCOPED_CRASH_KEY_*`) around the code
that may crash, instead of logging the state with `LOG()`.

**Why.** Crash reports do not include the user's logs: a crash key travels with
the report that needs it, and does not add noise to logs the rest of the time.

## Examples

Bad:

```cpp
LOG(ERROR) << "Unexpected state: " << state;
```

Good:

```cpp
static auto* crash_key = base::debug::AllocateCrashKeyString(
    "my_state", base::debug::CrashKeySize::Size64);
base::debug::ScopedCrashKeyString scoped_key(crash_key, state);
```
