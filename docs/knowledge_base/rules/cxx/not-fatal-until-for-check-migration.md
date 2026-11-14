---
name: brave/not-fatal-until-for-check-migration
description:
  'A DCHECK turned into a fatal CHECK or NOTREACHED where it may fire in the
  field can pass base::NotFatalUntil for an upcoming milestone to gather crash
  reports first.'
rubric: correctness
severity: optional
---

**Rule.** When a change turns a `DCHECK()` into a fatal `CHECK()` or
`NOTREACHED()` and it is not certain that the condition never fails in the
field, consider passing `base::NotFatalUntil::M<n>`, which reports a crash dump
without crashing until milestone `n`. Choose a milestone that has not shipped
yet: past milestones are removed from `base/not_fatal_until.h`.

**Why.** Promoting a long-standing `DCHECK()` to a fatal check can crash users
on paths no one has seen fail in release builds. `NotFatalUntil` gathers
diagnostics from the field first, then enforces the invariant.

## Examples

Bad:

```cpp
-  DCHECK(entry);
+  CHECK(entry);  // Never verified in release builds before.
```

Good:

```cpp
-  DCHECK(entry);
+  CHECK(entry, base::NotFatalUntil::M161);
```
