---
name: brave/default-initialize-pod-members
description:
  'Plain-old-data members (integers, floating-point values, bools, raw pointers)
  of structs and classes declared in headers must have a default member
  initializer.'
rubric: correctness
severity: blocking
---

**Rule.** Every data member of a scalar type (`int`, `size_t`, fixed-width
integers, `bool`, `float`, `double`, enums, raw pointers) declared in a struct
or class in a header MUST have a default member initializer, such as
`int id = 0;`, `double score = 0.0;`, `bool enabled = false;` or
`Foo* ptr = nullptr;`.

**Why.** A default-constructed object leaves scalar members without an
initializer holding indeterminate values; reading one is undefined behaviour,
and in practice yields garbage that differs between builds and runs.

## Examples

Bad:

```cpp
struct TopicArticle {
  int id;
  double score;
};
```

Good:

```cpp
struct TopicArticle {
  int id = 0;
  double score = 0.0;
};
```
