---
name: brave/match-crash-primitive-to-intent
description:
  'CHECK and NOTREACHED are used only for invariants; a failure still under
  investigation uses base::debug::DumpWithoutCrashing(), and a process that must
  die for outside reasons uses base::ImmediateCrash().'
rubric: correctness
severity: should_fix
---

**Rule.** Pick the primitive that matches the intent:

- `CHECK(cond)`, fatal in all builds, for invariants within the code's control;
- `DCHECK(cond)`, debug-only, for invariants too expensive to verify in
  production;
- `NOTREACHED()` for code that is genuinely unreachable;
- `base::ImmediateCrash()` when the process must die for reasons outside its
  control;
- `base::debug::DumpWithoutCrashing()`, which uploads a report and keeps
  running, to investigate a failure without killing the browser.

Do not `CHECK()` a condition that is still being diagnosed: report it with
`DumpWithoutCrashing()` and handle the failure.

**Why.** A `CHECK()` added to learn whether something happens crashes every user
it happens to; a crash dump gathers the same information while the browser keeps
running.

## Examples

Bad:

```cpp
// Not known to be an invariant yet.
CHECK(entry_was_found);
```

Good:

```cpp
if (!entry_was_found) {
  base::debug::DumpWithoutCrashing();
  return std::nullopt;
}
```
