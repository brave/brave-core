---
name: brave/size-t-for-sizes-and-indices
description:
  'Object and allocation sizes, element counts, array offsets and vector indices
  are size_t, also in public APIs, rather than int with casts around STL calls.'
rubric: correctness
severity: should_fix
---

**Rule.** Declare variables, parameters, fields and return values that hold a
size, a count of elements, an offset into an array or an index into a vector as
`size_t`. Do not declare them as `int` (or another signed or narrower type) and
`static_cast` the result of `.size()` into them, nor loop over a container with
an `int` index. A public API uses `size_t` even when its implementation stores
`uint32_t` internally. The rule concerns sizes, counts and positions of objects
and containers that exist at one time, not totals accumulated over a process's
lifetime (such as bytes allocated so far), which can exceed a 32-bit `size_t`.

**Why.** Container sizes are `size_t`: storing them in `int` needs casts at
every boundary, compares signed with unsigned, and truncates large values. Using
`size_t` throughout matches the STL and needs no casts.

## Examples

Bad:

```cpp
int count = static_cast<int>(vec.size());
for (int i = 0; i < vec.size(); ++i) { ... }
```

Good:

```cpp
size_t count = vec.size();
for (size_t i = 0; i < vec.size(); ++i) { ... }
```
