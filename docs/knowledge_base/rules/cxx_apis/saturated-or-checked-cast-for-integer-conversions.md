---
name: brave/saturated-or-checked-cast-for-integer-conversions
description:
  'Convert between integer types with base::saturated_cast<T>() when clamping is
  a valid result or base::checked_cast<T>() when the value cannot be out of
  range, combining saturated_cast with .value_or() for optional values.'
rubric: safety
severity: should_fix
---

**Rule.** When a value of one integer type is converted to another that may not
hold it, use a `base/numerics` cast chosen by what an out-of-range value means:

- `base::saturated_cast<T>()` when clamping to `T`'s minimum or maximum is an
  acceptable result;
- `base::checked_cast<T>()` when the value is an invariant that cannot be out of
  range, so a violation should `CHECK`-fail.

For an optional numeric value, write
`base::saturated_cast<T>(value.value_or(default))` instead of a `has_value()`
check followed by a `static_cast`.

**Why.** A `static_cast` between integer types silently wraps or truncates out
of range values. The named casts make the out-of-range behaviour explicit and
safe, and `value_or()` folds the presence check into the same expression.

## Examples

Bad:

```cpp
if (value.has_value()) {
  result = static_cast<uint64_t>(*value);
}
```

Good:

```cpp
result = base::saturated_cast<uint64_t>(value.value_or(0));
uint8_t byte = base::checked_cast<uint8_t>(value_known_to_be_0_255);
```
