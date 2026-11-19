---
name: brave/views-reverse-over-reverse-iterators
description:
  'Iterate a container backwards with a range-based for loop over
  std::views::reverse(container), not with explicit rbegin()/rend() iterators.'
rubric: readability
severity: nit
---

**Rule.** To visit the elements of a container in reverse order, write
`for (const auto& entry : std::views::reverse(history))`. Do not write a loop
over `rbegin()`/`rend()` (or `crbegin()`/`crend()`) iterators. Call
`std::views::reverse()` as a function: Chromium bans the `|` pipe syntax.

**Why.** The range-based loop states the traversal in one expression and has no
iterator to declare, compare or increment wrongly. `std::views::reverse` is one
of the range adaptors Chromium explicitly allows.

## Examples

Bad:

```cpp
for (auto it = history.crbegin(); it != history.crend(); ++it) {
  ProcessEntry(*it);
}
```

Good:

```cpp
// Process newest entries first to prioritize recent content.
for (const auto& entry : std::views::reverse(history)) {
  ProcessEntry(entry);
}
```
