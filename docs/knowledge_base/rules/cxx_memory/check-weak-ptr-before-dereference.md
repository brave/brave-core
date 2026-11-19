---
name: brave/check-weak-ptr-before-dereference
description:
  'Code must check that a base::WeakPtr is valid before dereferencing it,
  especially after an asynchronous operation.'
rubric: safety
severity: blocking
---

**Rule.** Before dereferencing a `base::WeakPtr` (`->`, `*` or `.get()`), check
that it is valid (`if (!weak_ptr) return;`), especially in a method that runs
after an asynchronous operation. NEVER assume the pointee is still alive.

**Why.** A `WeakPtr` becomes null when its target is destroyed or its factory
invalidated, which can happen at any point while an asynchronous operation is
pending. Dereferencing it without a check crashes on a null pointer.

## Examples

Bad:

```cpp
void OnCallback() {
  request_->Complete(result);  // request_ could be invalid!
}
```

Good:

```cpp
void OnCallback() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!request_)
    return;
  request_->Complete(result);
}
```
