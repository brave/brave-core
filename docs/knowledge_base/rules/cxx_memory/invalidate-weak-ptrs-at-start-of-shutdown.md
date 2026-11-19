---
name: brave/invalidate-weak-ptrs-at-start-of-shutdown
description:
  'A class with a WeakPtrFactory calls weak_factory_.InvalidateWeakPtrs() at the
  start of its shutdown or cleanup method.'
rubric: safety
severity: should_fix
---

**Rule.** In a class that hands out `base::WeakPtr`s from a
`base::WeakPtrFactory`, its shutdown or cleanup method (such as
`KeyedService::Shutdown()`) MUST call `weak_factory_.InvalidateWeakPtrs()`
first, before it releases any resources.

**Why.** Shutdown leaves the object alive but partially torn down. Callbacks
already bound to its `WeakPtr`s would otherwise still run afterwards and touch
the resources the shutdown released.

## Examples

Bad:

```cpp
void MyClass::Shutdown() {
  CleanupResources();
}
```

Good:

```cpp
void MyClass::Shutdown() {
  weak_factory_.InvalidateWeakPtrs();
  CleanupResources();
}
```
