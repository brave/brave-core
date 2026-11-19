---
name: brave/ranges-algorithms-over-search-loops
description:
  'Replace hand-written loops that search with a flag and break (or return
  early) by std::ranges::any_of, all_of, none_of, find_if and the other
  std::ranges algorithms.'
rubric: readability
severity: should_fix
---

**Rule.** When a loop only answers "does any/every element match?" or "which
element matches first?" (setting a flag and breaking, or returning `true`/
`false` from inside the loop), write it as `std::ranges::any_of()`,
`std::ranges::all_of()`, `std::ranges::none_of()`, `std::ranges::find_if()` or
`std::ranges::find()` with a predicate or projection. To check for a key in a
map or set, use the container's own `contains()` or `find()` instead.

**Why.** The algorithm names the question being asked, needs no mutable flag and
no `break`, and cannot get the early-exit logic wrong.

## Examples

Bad:

```cpp
bool found = false;
for (const auto& item : items) {
  if (item.IsExpired()) {
    found = true;
    break;
  }
}
```

Good:

```cpp
bool found = std::ranges::any_of(
    items, [](const auto& item) { return item.IsExpired(); });
```
