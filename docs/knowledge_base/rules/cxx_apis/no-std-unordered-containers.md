---
name: brave/no-std-unordered-containers
description:
  'std::unordered_map and std::unordered_set are banned; use
  absl::flat_hash_map/absl::flat_hash_set (or another container from the
  container guidelines) instead.'
rubric: perf
severity: blocking
---

**Rule.** NEVER add a `std::unordered_map`, `std::unordered_set`,
`std::unordered_multimap` or `std::unordered_multiset`. For an unordered
container use `absl::flat_hash_map`/`absl::flat_hash_set`, or the container that
[brave/choose-associative-container-by-use] picks for the case.

**Why.** Chromium's container guidelines (`base/containers/README.md`) never
recommend the standard unordered containers: they provide what the Abseil flat
hash containers do with worse performance and much higher memory overhead per
item, and are only justified for compatibility with third-party code.

## Examples

Bad:

```cpp
std::unordered_map<std::string, double> feature_map_;
```

Good:

```cpp
absl::flat_hash_map<std::string, double> feature_map_;
```
