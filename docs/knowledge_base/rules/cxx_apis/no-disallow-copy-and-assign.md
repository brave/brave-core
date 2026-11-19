---
name: brave/no-disallow-copy-and-assign
description:
  'New code must not use the deprecated DISALLOW_COPY_AND_ASSIGN macro; it
  deletes the copy constructor and copy assignment operator explicitly.'
rubric: readability
severity: should_fix
---

**Rule.** Do not add `DISALLOW_COPY_AND_ASSIGN(Type)`. Declare
`Type(const Type&) = delete;` and `Type& operator=(const Type&) = delete;` in
the `public:` section, next to the constructors.

**Why.** The macro is deprecated in Chromium. The explicit deleted declarations
are standard C++, are found where readers look for the constructors, and give
clearer compiler errors.

## Examples

Bad:

```cpp
class MyClass {
 private:
  DISALLOW_COPY_AND_ASSIGN(MyClass);
};
```

Good:

```cpp
class MyClass {
 public:
  MyClass(const MyClass&) = delete;
  MyClass& operator=(const MyClass&) = delete;
};
```
