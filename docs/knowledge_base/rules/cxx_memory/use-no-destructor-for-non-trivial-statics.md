---
name: brave/use-no-destructor-for-non-trivial-statics
description:
  'Global or static objects with non-trivial destructors are prohibited; use a
  function-local static base::NoDestructor, or constexpr for simple values.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER define a global or static object whose type has a non-trivial
destructor, such as a `std::map`, `std::vector` or `std::string`. When a global
or static container is needed, return it from a function that holds it in a
local `static const base::NoDestructor<T>`. Use `constexpr` for simple arrays
and values where possible.

**Why.** Chromium prohibits global objects with non-trivial destructors: they
run static initializers at startup and destructors at exit, in an order no code
controls. `base::NoDestructor` constructs the object on first use and never
destroys it.

## Examples

Bad:

```cpp
static const std::map<std::string, int> kMyLookup = {{"foo", 1}, {"bar", 2}};
```

Good:

```cpp
const std::map<std::string, int>& GetMyLookup() {
  static const base::NoDestructor<std::map<std::string, int>> lookup(
      {{"foo", 1}, {"bar", 2}});
  return *lookup;
}
```
