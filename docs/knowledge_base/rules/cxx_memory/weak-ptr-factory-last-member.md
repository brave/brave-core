---
name: brave/weak-ptr-factory-last-member
description:
  'A base::WeakPtrFactory member must be the last member declared in its class.'
rubric: safety
severity: should_fix
---

**Rule.** Declare the `base::WeakPtrFactory<T>` member last in the class. When
adding a member to a class that has one, add it before the factory.

**Why.** Members are destroyed in reverse declaration order. With the factory
last, it is destroyed first, so its `WeakPtr`s are invalidated before any other
member is torn down and no callback bound to them can run against a
half-destroyed object.

## Examples

Bad:

```cpp
class MyClass {
  base::WeakPtrFactory<MyClass> weak_factory_{this};
  std::unique_ptr<Fetcher> fetcher_;
};
```

Good:

```cpp
class MyClass {
  std::unique_ptr<Fetcher> fetcher_;
  base::WeakPtrFactory<MyClass> weak_factory_{this};  // must be last member
};
```
