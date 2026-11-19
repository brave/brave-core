---
name: brave/declare-members-in-dependency-order
description:
  'A member that another member depends on (views, pointers or observers into
  it) is declared first, so that it is destroyed last.'
rubric: safety
severity: blocking
---

**Rule.** When one member of a class refers into another (a `string_view`,
`span`, `raw_ptr`, iterator or observer pointing at data another member owns),
declare the member it depends on before it. Check the declaration order of every
such pair a change adds or reorders.

**Why.** C++ destroys members in reverse declaration order. If the owner is
declared after its dependent, the owner is destroyed first and the dependent
holds dangling references while it is itself destroyed, which is a
use-after-free.

## Examples

Bad:

```cpp
class NameTable {
  // map_ holds string_views into strings_, but strings_ is destroyed first.
  base::flat_map<std::string_view, NameId> map_;
  std::deque<std::string> strings_;
};
```

Good:

```cpp
class NameTable {
  std::deque<std::string> strings_;
  base::flat_map<std::string_view, NameId> map_;
};
```
