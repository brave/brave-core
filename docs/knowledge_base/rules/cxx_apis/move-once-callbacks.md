---
name: brave/move-once-callbacks
description:
  'A base::OnceCallback is passed on and run with std::move:
  std::move(callback).Run(...) and Foo(std::move(callback)).'
rubric: correctness
severity: should_fix
---

**Rule.** Run a `base::OnceCallback` (or `base::OnceClosure`) as
`std::move(callback).Run(...)`, and hand it to another function, a bound task or
a member with `std::move(callback)`. Do not try to copy a once callback or keep
using it after it has been moved from.

**Why.** A once callback is move-only and is consumed when it runs: running it
through an rvalue makes that consumption visible at the call site, and moving it
when it is passed on transfers the single right to run it. Code that keeps a
moved-from callback around and runs it later crashes or silently does nothing.

## Examples

Bad:

```cpp
void FooService::OnFetched(base::OnceCallback<void(bool)> callback,
                           bool success) {
  PostDone(callback);
  callback.Run(success);
}
```

Good:

```cpp
void FooService::OnFetched(base::OnceCallback<void(bool)> callback,
                           bool success) {
  std::move(callback).Run(success);
}
```
