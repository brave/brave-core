---
name: brave/find-or-null-for-map-lookups
description:
  'Look up a map value that may be missing with base::FindOrNull(map, key), not
  with find() and a comparison against end().'
rubric: readability
severity: nit
---

**Rule.** When code looks up a key and only needs a pointer to the value (or
null when the key is absent), call `base::FindOrNull(map, key)` from
`base/containers/map_util.h`. Do not write `auto it = map.find(key);` followed
by `it == map.end()` and `&it->second`. Keep the iterator form only when the
iterator itself is needed, for example to erase the entry.

**Why.** The helper replaces three statements with one expression and removes
the chance of comparing against the wrong container's `end()` or dereferencing
an unchecked iterator.

## Examples

Bad:

```cpp
auto it = metric_configs_.find(metric_name);
if (it == metric_configs_.end()) {
  return nullptr;
}
return &it->second;
```

Good:

```cpp
return base::FindOrNull(metric_configs_, metric_name);
```
