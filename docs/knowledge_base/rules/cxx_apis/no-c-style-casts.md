---
name: brave/no-c-style-casts
description:
  'C-style casts such as (double)x or (Foo*)ptr are prohibited; use static_cast,
  reinterpret_cast, const_cast or a base/numerics conversion.'
rubric: safety
severity: blocking
---

**Rule.** NEVER write a C-style cast `(T)expr`. Use the C++ cast that states the
conversion: `static_cast<T>()` for value conversions, `reinterpret_cast<T>()`
for reinterpreting bits or pointers, `const_cast<T>()` for removing `const`, or
a `base/numerics` helper such as `base::checked_cast<T>()` for integer
narrowing. A cast to `void` that discards a value is not covered by this rule.

**Why.** A C-style cast silently picks whichever of `static_cast`, `const_cast`
and `reinterpret_cast` makes the expression compile, so it can drop `const` or
reinterpret unrelated types without any sign at the call site. The named casts
make the intent explicit and let the compiler reject the conversions they do not
allow.

## Examples

Bad:

```cpp
double result = (double)integer_value / total;
```

Good:

```cpp
double result = static_cast<double>(integer_value) / total;
```
