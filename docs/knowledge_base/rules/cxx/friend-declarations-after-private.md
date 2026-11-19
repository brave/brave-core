---
name: brave/friend-declarations-after-private
description:
  'friend declarations go immediately after the private: access specifier,
  before any member variables or methods.'
rubric: readability
severity: nit
---

**Rule.** Place `friend` declarations (including `FRIEND_TEST_ALL_PREFIXES`)
first in the `private:` section of a class, before its methods and member
variables, followed by a blank line.

**Why.** Grouped at the top of the private section, the friends are where a
reader looks for who may reach the private members; buried among them, they are
missed.

## Examples

Bad:

```cpp
class MyClass {
 private:
  int value_ = 0;
  friend class MyClassTest;
};
```

Good:

```cpp
class MyClass {
 private:
  friend class MyClassTest;

  int value_ = 0;
};
```
