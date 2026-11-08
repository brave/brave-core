---
name: brave/constexpr-for-compile-time-constants
description:
  'A constant whose value is known at compile time is declared constexpr, not
  const.'
rubric: readability
severity: should_fix
---

**Rule.** Declare a new constant with `constexpr`
(`constexpr int kMaxRetries = 3;`, `constexpr char kName[] = "...";`) whenever
its initialiser is a compile-time constant. Keep `const` only for values that
cannot be computed at compile time, and for `extern` declarations whose
definition lives elsewhere.

**Why.** `constexpr` guarantees compile-time initialisation, so the constant can
be used in other constant expressions and never runs a static initialiser. A
`const` with a constant initialiser gives the same value with a weaker guarantee
the reader has to verify.

## Examples

Bad:

```cpp
namespace {
const int kMaxRetries = 3;
}  // namespace
```

Good:

```cpp
namespace {
constexpr int kMaxRetries = 3;
}  // namespace
```
