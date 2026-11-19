---
name: brave/comment-early-return-failure-branches
description:
  'A function with several early-return failure branches has a brief comment
  before each saying what it guards against.'
rubric: readability
severity: nit
---

**Rule.** When a function bails out early on failure in more than one place, put
a short comment before each such branch saying what it rejects and why the input
is required, as in the example. Do not merely restate the condition.

**Why.** A run of bare `if (!x) return std::nullopt;` lines does not tell the
reader what each check protects, or which failure a given return reports.

## Examples

Bad:

```cpp
if (!parent_hash) {
  return std::nullopt;
}
if (!state_root) {
  return std::nullopt;
}
if (!number) {
  return std::nullopt;
}
```

Good:

```cpp
// Parent block hash is required for chain continuity.
if (!parent_hash) {
  return std::nullopt;
}
// State root validates the block's state trie.
if (!state_root) {
  return std::nullopt;
}
// Block number must be present and valid.
if (!number) {
  return std::nullopt;
}
```
