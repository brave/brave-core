---
name: brave/hash-map-pointer-stability
description:
  'When pointers into a hash map must stay valid, wrap values in std::unique_ptr
  inside absl::flat_hash_map for value stability, and use
  absl::node_hash_map/absl::node_hash_set only for key stability.'
rubric: perf
severity: should_fix
---

**Rule.** When code keeps pointers or references to elements of a hash container
across insertions:

- For stable values, use `absl::flat_hash_map<Key, std::unique_ptr<Value>>`.
- For stable keys, use `absl::node_hash_map`/`absl::node_hash_set`.

Do not reach for `absl::node_hash_map` when only the values need stable
addresses, and do not keep pointers into a plain `absl::flat_hash_map`, whose
elements move when it rehashes.

**Why.** The flat hash containers store elements inline and move them on rehash,
so pointers into them dangle. Wrapping only the values keeps the flat layout for
everything else; the node containers allocate every entry separately, a cost
worth paying only when keys must not move.

## Examples

Bad:

```cpp
absl::flat_hash_map<Key, Value> values_;
Value* cached_ = &values_[key];  // Dangles after the next rehash.
```

Good:

```cpp
absl::flat_hash_map<Key, std::unique_ptr<Value>> stable_values_;
absl::node_hash_map<std::string, int> stable_keys_;
```
