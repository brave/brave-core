---
name: brave/constexpr-time-delta-constants
description: 'A base::TimeDelta constant is declared constexpr, not just const.'
rubric: readability
severity: nit
---

**Rule.** Declare a time constant as
`constexpr base::TimeDelta kName = base::Seconds(...)`, not
`const base::TimeDelta` or `const auto`.

**Why.** `base::Seconds()`, `base::Milliseconds()` and the other factories are
`constexpr`, so the constant can be computed at compile time instead of being
initialised at run time.

## Examples

Bad:

```cpp
const base::TimeDelta kAnimationDuration = base::Milliseconds(200);
```

Good:

```cpp
constexpr base::TimeDelta kAnimationDuration = base::Milliseconds(200);
```
