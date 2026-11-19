---
name: brave/fallback-calls-correct-base-class
description:
  'The fallback in a chromium_src override method calls the upstream
  implementation through the right class (ChromiumImpl after a #define rename),
  not the renamed original.'
rubric: correctness
severity: blocking
---

**Rule.** When an override method falls back to the upstream implementation,
check which class each name refers to after the override's `#define` renames,
and call the class that holds the upstream code: `ChromiumImpl::` (or
`Method_ChromiumImpl()`) where the override renamed the original. NEVER call the
original class name when a rename has made it refer to the Brave class or to
something else.

**Why.** A `#define` rename changes the class hierarchy the code sees, so a
fallback written against the original name calls a different method than the
upstream implementation it means to reach.

## Examples

Bad:

```cpp
SkColor BraveProvider::GetColor(int id) {
  return ChromeTypographyProvider::GetColor(id);
}
```

Good:

```cpp
SkColor BraveProvider::GetColor(int id) {
  return ChromiumImpl::GetColor(id);
}
```
