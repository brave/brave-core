---
name: brave/no-default-member-init-when-ctor-sets
description:
  'A member that every constructor sets explicitly gets no default initializer
  in its declaration.'
rubric: readability
severity: nit
---

**Rule.** Initialise class members in their declarations, except a member that
every constructor sets explicitly: leave that one without a default initializer,
such as `= nullptr`.

**Why.** A default that no object ever keeps is misleading: it suggests the
member can hold that value, so readers add checks for a state that cannot
happen.

## Examples

Bad:

```cpp
class TreeTabNode {
 public:
  explicit TreeTabNode(TabInterface* tab);  // Always sets current_tab_.

 private:
  raw_ptr<TabInterface> current_tab_ = nullptr;
};
```

Good:

```cpp
class TreeTabNode {
 public:
  explicit TreeTabNode(TabInterface* tab) : current_tab_(tab) {}

 private:
  raw_ptr<TabInterface> current_tab_;
};
```
