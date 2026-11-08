---
name: brave/erase-if-over-manual-erase-loops
description:
  'Remove matching elements with base::EraseIf() (base::flat_* containers) or
  std::erase_if() (standard containers), not with a hand-written iterator erase
  loop.'
rubric: readability
severity: should_fix
---

**Rule.** To remove every element that matches a predicate, call
`base::EraseIf(container, pred)` for a `base::flat_map`/`base::flat_set` (or
`base::circular_deque`) and `std::erase_if(container, pred)` for a standard
container. Do not write a loop that advances an iterator and assigns
`it = container.erase(it)`, nor the `erase(remove_if(...), end())` idiom.

**Why.** Manual erase loops are easy to get wrong (incrementing an invalidated
iterator, skipping the element after an erased one) and, on a flat or contiguous
container, erasing one element at a time is O(n²). The helpers remove all
matches in a single O(n) pass.

## Examples

Bad:

```cpp
for (auto it = items.begin(); it != items.end();) {
  if (it->IsExpired()) {
    it = items.erase(it);
  } else {
    ++it;
  }
}
```

Good:

```cpp
std::erase_if(items, [](const auto& item) { return item.IsExpired(); });
```
