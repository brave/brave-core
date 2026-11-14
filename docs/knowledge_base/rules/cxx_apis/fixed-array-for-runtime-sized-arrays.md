---
name: brave/fixed-array-for-runtime-sized-arrays
description:
  'An array whose size is known when it is created but not at compile time, and
  that is never resized, is a base::FixedArray rather than a std::vector.'
rubric: perf
severity: should_fix
---

**Rule.** When a local or member array is created with a runtime size and is
never resized afterwards (no `push_back()`, `resize()`, `insert()` or
`erase()`), declare it as `base::FixedArray<T>` (`base/types/fixed_array.h`)
instead of `std::vector<T> v(size)`. For an owned byte buffer, see
[brave/heap-array-for-byte-buffers]. Use `std::array` when the size is a
compile-time constant. Do not use `absl::FixedArray` directly, which Chromium
bans.

**Why.** `base::FixedArray` stores small arrays inline, avoiding a heap
allocation, and its type tells the reader that the size does not change. A
`std::vector` suggests the array grows.

## Examples

Bad:

```cpp
std::vector<uint8_t> out(size);
```

Good:

```cpp
base::FixedArray<uint8_t> out(size);
```
