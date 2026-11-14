---
name: brave/contiguous-containers-over-std-list
description:
  'Never use std::list for data that is only traversed; use std::vector
  (reserved when the size is known) and keep std::list for stable iterators or
  frequent mid-container insertion and removal.'
rubric: perf
severity: blocking
---

**Rule.** NEVER add a `std::list` (or `std::forward_list`) that is only iterated
over, appended to, or read from the ends. Use a `std::vector`, calling
`reserve()` first when the final size is known. A `std::list` is justified only
when the code relies on iterators that stay valid across mutations, or inserts
and removes in the middle of the sequence frequently.

**Why.** Each list node is a separate heap allocation, so traversing a list
chases pointers across memory with poor cache locality, and every insertion
allocates. A vector stores its elements contiguously and is faster for almost
every traversal-heavy workload.

## Examples

Bad:

```cpp
std::list<Entry> entries_;  // Only appended to and iterated.
```

Good:

```cpp
std::vector<Entry> entries_;
```
