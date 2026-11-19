---
name: brave/notreached-only-for-invariants
description:
  'NOTREACHED() and CHECK(false) crash the browser only for broken invariants;
  unexpected data such as an unknown enum value is handled with std::optional or
  a default.'
rubric: correctness
severity: blocking
---

**Rule.** Use `NOTREACHED()` or `CHECK(false)` only where reaching the line
means the program's own invariants are broken. NEVER use them for values that
come from data the code processes (an unexpected string or enum value read from
storage, a server or a component): return `std::optional` / `std::nullopt` so
the caller handles the failure, or a default value.

**Why.** `NOTREACHED()` is fatal in all builds: one unexpected input crashes the
user's browser over what is not a bug in the code.

## Examples

Bad:

```cpp
mojom::AdType ToMojomAdType(const std::string& type) {
  // ...
  NOTREACHED();
}
```

Good:

```cpp
std::optional<mojom::AdType> ToMojomAdType(const std::string& type) {
  // ...
  return std::nullopt;
}
```
