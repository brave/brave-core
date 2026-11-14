---
name: brave/no-static-in-anonymous-namespace
description:
  'A constexpr constant inside an anonymous namespace is not also declared
  static.'
rubric: readability
severity: nit
---

**Rule.** Do not write `static constexpr` for a constant at the scope of an
anonymous namespace; write `constexpr`. This is about namespace scope only:
`static constexpr` stays correct for class members and for constants local to a
function.

**Why.** The anonymous namespace already gives the constant internal linkage, so
`static` adds nothing but noise.

## Examples

Bad:

```cpp
namespace {
static constexpr int kMaxRetries = 3;
}  // namespace
```

Good:

```cpp
namespace {
constexpr int kMaxRetries = 3;
}  // namespace
```
