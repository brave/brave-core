---
name: brave/use-mojom-types-instead-of-duplicate-structs
description:
  'Where a mojom type already describes the data, C++ uses it directly instead
  of defining a parallel C++ struct.'
rubric: design
severity: should_fix
---

**Rule.** Before adding a C++ struct, check whether a mojom struct already
describes the same data; if so, use the mojom type (`mojom::FooPtr`) in C++
rather than defining a struct with the same fields and converting between them.

**Why.** Two definitions of one data shape must be kept in sync by hand, and
they drift apart.

## Examples

Bad:

```cpp
struct ToolConfig {
  std::string name;
  std::string description;
};
```

Good:

```cpp
// mojom::ToolConfig already has name and description fields
void RegisterTool(mojom::ToolConfigPtr config);
```
