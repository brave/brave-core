---
name: brave/document-and-guard-reentrancy
description:
  'Where re-entrancy (a callback calling back into the same object) is
  unavoidable, it is documented and guarded, for example with base::AutoReset,
  against infinite recursion.'
rubric: correctness
severity: should_fix
---

**Rule.** Avoid designs in which a callback or notification calls back into the
object that triggered it. Where that cannot be avoided, document the re-entrant
path in a comment, and guard it, for example with a flag set through
`base::AutoReset` for the duration of the call, so that it cannot recurse
indefinitely.

**Why.** Re-entrant code runs with the object's state half-updated, and an
unguarded cycle recurses until the stack overflows.

## Examples

Bad:

```cpp
void Foo::Update() {
  for (auto& observer : observers_) {
    observer.OnUpdated();  // an observer may call Update() again
  }
}
```

Good:

```cpp
void Foo::Update() {
  // Observers may call Update() again; nested calls are ignored.
  if (updating_) {
    return;
  }
  base::AutoReset<bool> updating(&updating_, true);
  for (auto& observer : observers_) {
    observer.OnUpdated();
  }
}
```
