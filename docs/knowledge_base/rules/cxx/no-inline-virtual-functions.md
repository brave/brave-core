---
name: brave/no-inline-virtual-functions
description:
  'A virtual function, including an override, is never defined inline in its
  class declaration.'
rubric: readability
severity: should_fix
---

**Rule.** NEVER define a `virtual` function, or an `override` of one, in the
body of its class in a header, even when it only returns a member. Declare it
there and define it in the `.cc` file. The simple-accessor exception does not
extend to virtual functions.

**Why.** A virtual call is dispatched at run time, so defining it inline saves
nothing, while it still puts the body, and its dependencies, into every file
including the header. Chromium's style guide forbids it.

## Examples

Bad:

```cpp
class Handler {
  virtual bool IsEnabled() const { return enabled_; }
};
```

Good:

```cpp
// handler.h
class Handler {
  virtual bool IsEnabled() const;
};

// handler.cc
bool Handler::IsEnabled() const {
  return enabled_;
}
```
