---
name: brave/transparent-lookups-in-flat-containers
description:
  'Look up base::flat_map/base::flat_set entries with find(), contains() or
  count() on a std::string_view or literal, which compare transparently, rather
  than operator[] or a temporary std::string; use base::UniquePtrComparator for
  sets of std::unique_ptr.'
rubric: perf
severity: nit
---

**Rule.** To read an entry of a `base::flat_map<std::string, V>` or
`base::flat_set<std::string>`, call `find()`, `contains()` or `count()` with the
`std::string_view` or literal directly. Do not use `operator[]` for a lookup,
and do not wrap the key in `std::string(...)` to search. A
`base::flat_set<std::unique_ptr<T>>` that is searched by raw pointer declares
`base::UniquePtrComparator` as its comparator.

**Why.** The flat containers compare keys transparently, so a view or literal is
compared without allocating. `operator[]` and an explicit `std::string(key)`
construct a temporary string on every lookup, and `operator[]` also inserts a
default value when the key is missing.

## Examples

Bad:

```cpp
base::flat_map<std::string, int> map;
int val = map["key"];  // temporary std::string("key") created
```

Good:

```cpp
base::flat_map<std::string, int> map;
auto it = map.find("key");  // no temporary string created
```
