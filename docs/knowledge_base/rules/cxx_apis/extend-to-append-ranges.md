---
name: brave/extend-to-append-ranges
description:
  'Append one collection to a vector with base::Extend(target, source), not
  target.insert(target.end(), source.begin(), source.end()).'
rubric: readability
severity: nit
---

**Rule.** To append all elements of a range to a `std::vector`, call
`base::Extend(target, source)` from `base/containers/extend.h` (passing
`std::move(source)` to move the elements, or a `base::span` for arrays). Do not
write `target.insert(target.end(), source.begin(), source.end())`.

**Why.** `base::Extend()` names the operation and takes each range once; the
`insert()` form repeats both containers, which makes it easy to mix up `begin()`
and `end()` of the wrong one.

## Examples

Bad:

```cpp
accelerator_list.insert(accelerator_list.end(), brave_accelerators.begin(),
                        brave_accelerators.end());
```

Good:

```cpp
base::Extend(accelerator_list, base::span(kBraveAcceleratorMap));
```
