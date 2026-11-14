---
name: brave/no-absl-btree-containers
description:
  'absl::btree_map and absl::btree_set are banned in Chromium code because of
  their code size; use std::map/std::set or base::flat_map/base::flat_set for
  sorted containers.'
rubric: perf
severity: blocking
---

**Rule.** NEVER use `absl::btree_map`, `absl::btree_set`, `absl::btree_multimap`
or `absl::btree_multiset`. A sorted container is a
`base::flat_map`/`base::flat_set` when it is small or written once, and a
`std::map`/`std::set` when it is large and frequently mutated.

**Why.** The Abseil B-tree containers perform well, but they add a very large
code size penalty in Chromium, so Chromium's style guide bans them until that is
resolved.

## Examples

Bad:

```cpp
absl::btree_map<std::string, int> sorted_counts_;
```

Good:

```cpp
// Sorted, large and frequently mutated.
std::map<std::string, int> sorted_counts_;
```
