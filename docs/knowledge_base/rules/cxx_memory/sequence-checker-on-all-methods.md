---
name: brave/sequence-checker-on-all-methods
description:
  'A single-threaded class with a sequence checker applies
  DCHECK_CALLED_ON_VALID_SEQUENCE to all of its methods, not only some.'
rubric: correctness
severity: should_fix
---

**Rule.** When a single-threaded class has a `SEQUENCE_CHECKER`, every method
checks it with `DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)`. Flag a
method added or changed without the check in such a class, and do not resolve
partial checking by removing the checks.

**Why.** Partial checking is misleading: the checked methods suggest the class
is sequence-bound, and the unchecked ones then look unsafe even when they are
correct, while a call from the wrong sequence goes undetected.

## Examples

Bad:

```cpp
void MyClass::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  running_ = true;
}

void MyClass::Stop() {
  running_ = false;
}
```

Good:

```cpp
void MyClass::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  running_ = true;
}

void MyClass::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  running_ = false;
}
```
