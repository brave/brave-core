---
name: brave/composition-over-multiple-inheritance
description:
  'A class inherits from several classes only when the extra bases are
  interfaces; otherwise it uses composition.'
rubric: design
severity: nit
---

**Rule.** Inherit from more than one class only when all but one of them are
interfaces (observer and delegate interfaces, pure virtual classes). When a
class would inherit from several concrete classes, hold the extra ones as
members instead, unless composition cannot achieve the same.

**Why.** Inheriting from several concrete classes builds deep hierarchies with
tangled state and lifetimes; composition keeps each part separate and
replaceable.

## Examples

Bad:

```cpp
class FooController : public FooStore,
                      public BarFetcher { ... };
```

Good:

```cpp
class FooController : public FooStore::Observer {
 private:
  std::unique_ptr<FooStore> store_;
  std::unique_ptr<BarFetcher> fetcher_;
};
```
