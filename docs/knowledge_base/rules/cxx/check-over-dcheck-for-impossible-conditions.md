---
name: brave/check-over-dcheck-for-impossible-conditions
description:
  'A condition that must never happen in any build is asserted with CHECK, not
  DCHECK; DCHECK is kept for invariants too expensive to verify in production.'
rubric: correctness
severity: should_fix
---

**Rule.** Assert an invariant that must hold in every build with `CHECK()` (or
`CHECK_EQ()` and the like). Use `DCHECK()` only when verifying the invariant is
too expensive for release builds. A new `DCHECK()` of a cheap condition, such as
a null pointer, is written as a `CHECK()`.

**Why.** `DCHECK()` compiles away in release builds, so a violation there goes
on silently into undefined behaviour and never shows in crash reports. `CHECK()`
stops the process at the violation, in every build.

## Examples

Bad:

```cpp
DCHECK(browser_context);
```

Good:

```cpp
CHECK(browser_context);
```
