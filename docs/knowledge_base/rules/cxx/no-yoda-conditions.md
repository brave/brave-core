---
name: brave/no-yoda-conditions
description:
  'Comparisons put the variable first and the constant second, as in (foo == 0),
  not (0 == foo).'
rubric: readability
severity: nit
---

**Rule.** Write a comparison with the value under test on the left and the
constant on the right: `(foo == 0)`, `(ptr != nullptr)`, not `(0 == foo)` or
`(nullptr != ptr)`.

**Why.** It reads as the question being asked; the reversed form guarded against
accidental assignment, which compiler warnings now catch.

## Examples

Bad:

```cpp
if (0 == count) {
```

Good:

```cpp
if (count == 0) {
```
