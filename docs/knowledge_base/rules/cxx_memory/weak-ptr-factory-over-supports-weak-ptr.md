---
name: brave/weak-ptr-factory-over-supports-weak-ptr
description:
  'Classes vend WeakPtrs through a base::WeakPtrFactory<T> member, not by
  inheriting from base::SupportsWeakPtr<T>.'
rubric: design
severity: should_fix
---

**Rule.** To hand out `base::WeakPtr<T>`s, declare a `base::WeakPtrFactory<T>`
member. Do not inherit from `base::SupportsWeakPtr<T>`.

**Why.** `WeakPtrFactory` performs more safety checks and is the recommended
pattern.

## Examples

Bad:

```cpp
class MyClass : public base::SupportsWeakPtr<MyClass> {};
```

Good:

```cpp
class MyClass {
  base::WeakPtrFactory<MyClass> weak_factory_{this};  // must be last member
};
```
