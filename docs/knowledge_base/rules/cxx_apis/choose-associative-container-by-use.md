---
name: brave/choose-associative-container-by-use
description:
  'A new map or set uses the container the Chromium container guidelines give
  for its use: absl::flat_hash_* by default, base::flat_* for small or
  write-once sorted data, std::map/std::set for large frequently mutated sorted
  data.'
rubric: perf
severity: should_fix
---

**Rule.** Pick an associative container by how it is used:

- No ordering needed: `absl::flat_hash_map`/`absl::flat_hash_set`.
- Sorted, and small or written once: `base::flat_map`/`base::flat_set`. Their
  O(n) insertions and removals are fine at that size.
- Sorted, large and frequently mutated: `std::map`/`std::set`.
- Known at compile time: `base::MakeFixedFlatMap`/`base::MakeFixedFlatSet` (see
  [brave/fixed-flat-map-for-constant-lookup-tables]).

Flag a container whose declared use does not match its type, such as a
`std::map` that needs no ordering, or a `base::flat_map` that grows by repeated
insertions into a large collection.

**Why.** The containers differ widely in memory overhead, cache locality and the
cost of mutation. Chromium's container guidelines (`base/containers/README.md`)
give this choice so that each container gets the best all-round performance for
its case.

## Examples

Bad:

```cpp
// Never iterated in order, only looked up.
std::map<std::string, double> feature_map_;
```

Good:

```cpp
absl::flat_hash_map<std::string, double> feature_map_;
base::flat_map<std::string, int> lookup_;           // sorted, small/write-once
std::map<std::string, int> large_mutable_lookup_;   // sorted, large, mutated
```
