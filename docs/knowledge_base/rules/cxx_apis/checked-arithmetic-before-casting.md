---
name: brave/checked-arithmetic-before-casting
description:
  'Arithmetic that could overflow is computed with base::CheckedNumeric (or the
  Check*() helpers) and read with AssignIfValid()/ValueOrDie(), not computed
  naked and cast afterwards.'
rubric: safety
severity: blocking
---

**Rule.** When a product, sum or difference can overflow its operand type, such
as a size computed from a count and an element size taken from input, it MUST be
computed with checked math (`base::CheckMul()`, `base::CheckAdd()`, or a
`base::CheckedNumeric<T>`) and read with `.AssignIfValid(&out)` (handling
failure) or `.ValueOrDie()`. Wrapping the naked expression in
`base::saturated_cast<T>()`, `base::checked_cast<T>()` or `static_cast<T>()`
does not make it safe.

**Why.** The overflow happens while the expression is evaluated, before any cast
sees the result: the cast then clamps or checks an already wrapped value.
Overflowed buffer sizes lead to undersized allocations and out-of-bounds
accesses.

## Examples

Bad:

```cpp
size_t total = base::saturated_cast<size_t>(count * item_size);
```

Good:

```cpp
size_t total = 0;
if (!base::CheckMul(count, item_size).AssignIfValid(&total)) {
  return std::nullopt;
}
```
