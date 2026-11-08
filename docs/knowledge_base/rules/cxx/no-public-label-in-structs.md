---
name: brave/no-public-label-in-structs
description:
  'A struct declaration has no public: label; it is removed, or the struct
  becomes a class if access control is intended.'
rubric: readability
severity: nit
---

**Rule.** Do not write `public:` in a `struct`. Remove the label, or, when the
type needs `private:` members, declare it as a `class`.

**Why.** Members of a struct are public by default, so the label is noise, and a
struct with access control is a class by Chromium's conventions.

## Examples

Bad:

```cpp
struct TestData {
 public:
  std::string name;
  int value;
};
```

Good:

```cpp
struct TestData {
  std::string name;
  int value;
};
```
