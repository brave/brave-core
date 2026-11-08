---
name: brave/constexpr-int-over-single-value-enum
description:
  'A single numeric constant is a constexpr int, not an enum with one
  enumerator.'
rubric: readability
severity: nit
---

**Rule.** Do not declare an enum, anonymous or named, with a single enumerator
just to define a numeric constant. Declare `constexpr int kName = value;` (or
another integer type that fits the use).

**Why.** An enum announces a set of related values; a one-value enum misleads
the reader about that, and gives the constant an enum type that converts to
integers implicitly or needs casts.

## Examples

Bad:

```cpp
enum { kBravePolicySource = 10 };
```

Good:

```cpp
constexpr int kBravePolicySource = 10;
```
