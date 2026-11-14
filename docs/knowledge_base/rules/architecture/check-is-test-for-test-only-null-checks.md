---
name: brave/check-is-test-for-test-only-null-checks
description:
  'A null check for a pointer that is null only in some test configurations
  begins its null branch with CHECK_IS_TEST().'
rubric: correctness
severity: should_fix
---

**Rule.** When production code null-checks a pointer that is never null in
production but can be in some test configurations, the branch handling null
starts with `CHECK_IS_TEST();`. Do not leave a bare early return that reads as
if null were a valid production state.

**Why.** `CHECK_IS_TEST()` documents that the null case exists only for tests,
and crashes if it ever happens in production instead of silently skipping the
work.

## Examples

Bad:

```cpp
if (!g_brave_browser_process->speedreader_rewriter_service()) {
  return;
}
```

Good:

```cpp
if (!service) {
  CHECK_IS_TEST();
  return;
}
```
