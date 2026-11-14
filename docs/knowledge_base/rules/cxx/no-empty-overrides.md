---
name: brave/no-empty-overrides
description:
  'A virtual method is not overridden just to do nothing; an override with an
  empty body is removed.'
rubric: readability
severity: should_fix
---

**Rule.** Do not declare or define an override whose body is empty. When the
class has nothing to do in a virtual method, do not override it at all, and rely
on the base class's implementation. Only a pure virtual method, which the class
must implement, can need an empty body.

**Why.** An empty override adds a declaration, a definition and a reader's
question about why it is there, for no behaviour.

## Examples

Bad:

```cpp
void OnSomethingHappened() override {}
```

Good:

```cpp
// No override: the base class's implementation does nothing either.
```
