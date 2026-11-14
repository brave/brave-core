---
name: brave/check-over-conditional-notreached
description:
  'An invariant is stated with an unconditional CHECK(), not an if whose body is
  NOTREACHED().'
rubric: readability
severity: nit
---

**Rule.** Write `CHECK(condition)` instead of
`if (!condition) { NOTREACHED(); }`.

**Why.** The `CHECK()` is shorter and states the invariant directly, where the
`if` makes the reader invert the condition to find it.

## Examples

Bad:

```cpp
if (!profile) {
  NOTREACHED();
}
```

Good:

```cpp
CHECK(profile);
```
