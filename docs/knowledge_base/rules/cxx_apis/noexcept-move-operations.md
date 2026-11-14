---
name: brave/noexcept-move-operations
description:
  'Custom or defaulted move constructors and move assignment operators of types
  stored in std::vector are declared noexcept.'
rubric: perf
severity: should_fix
---

**Rule.** When a change declares a move constructor or move assignment operator
(custom or `= default`) for a type that is stored in a `std::vector` or another
container that reallocates, declare it `noexcept`:
`Topic(Topic&&) noexcept = default;` and
`Topic& operator=(Topic&&) noexcept = default;`.

**Why.** `std::vector` only moves elements during reallocation when their move
constructor cannot throw; otherwise it copies each of them to keep its strong
exception guarantee. A move constructor declared in the header and defaulted in
the `.cc` file, as Chromium's style checker requires for complex classes, is
potentially throwing unless declared `noexcept`, so every growth of the vector
copies.

## Examples

Bad:

```cpp
Topic(Topic&&) = default;
Topic& operator=(Topic&&) = default;
```

Good:

```cpp
Topic(Topic&&) noexcept = default;
Topic& operator=(Topic&&) noexcept = default;
```
