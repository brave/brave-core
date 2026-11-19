---
name: brave/to-vector-for-range-conversions
description:
  'Convert a range to a std::vector with base::ToVector(range) (optionally with
  a projection), not with manual reserve() plus copy or back_inserter.'
rubric: readability
severity: should_fix
---

**Rule.** When a change builds a `std::vector` from an existing range (a
container, a span, a static array), or from a member of each of its elements,
use `base::ToVector(range)` or `base::ToVector(range, projection)`. Do not
`reserve()` a vector and fill it with `std::copy`/`std::copy_n` and
`std::back_inserter`, or with a loop of `push_back()` calls. Constructing the
vector directly from the range with `std::from_range` is not a manual copy
pattern.

**Why.** The manual pattern repeats the size computation, the reservation and
the iteration at every call site, and each of those is a chance for an
off-by-one or a forgotten `reserve()`. `base::ToVector` does all three correctly
in one call.

## Examples

Bad:

```cpp
std::vector<std::string> names;
names.reserve(items.size());
for (const auto& item : items) {
  names.push_back(item.name);
}
```

Good:

```cpp
auto names = base::ToVector(items, &Item::name);
```
