---
name: brave/comment-why-reverse-order
description:
  'Every loop that iterates in reverse order has a comment saying why the
  reverse order is needed.'
rubric: readability
severity: nit
---

**Rule.** When a change adds a reverse traversal (`std::views::reverse()`,
reverse iterators, or an index counting down), put a comment just before it
explaining why the elements must be visited from the back, such as "newest
first" or "so that erasing does not shift unvisited indices".

**Why.** Reverse order is usually load-bearing but rarely obvious: without the
reason, a later change may "simplify" the loop to forward order and silently
change which element wins or which entries are processed first.

## Examples

Bad:

```cpp
for (const auto& entry : std::views::reverse(history)) {
  ProcessEntry(entry);
}
```

Good:

```cpp
// Process newest entries first to prioritize recent content.
for (const auto& entry : std::views::reverse(history)) {
  ProcessEntry(entry);
}
```
