---
name: brave/sequence-check-weak-ptr-and-bound-methods
description:
  'Methods that use WeakPtrs or are bound as base::BindOnce/BindRepeating
  targets start with DCHECK_CALLED_ON_VALID_SEQUENCE.'
rubric: correctness
severity: should_fix
---

**Rule.** A method that is the target of `base::BindOnce` /
`base::BindRepeating`, or that uses a `base::WeakPtr`, should begin with
`DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)` (with a `SEQUENCE_CHECKER`
member in the class), so that a callback run on the wrong sequence fails in
debug builds.

**Why.** Bound callbacks are often run on a sequence other than the one that
created them, and a `WeakPtr` may only be checked and dereferenced on the
sequence it is bound to. The check turns a silent data race into an immediate
failure in tests.

## Examples

Bad:

```cpp
void MyService::OnGetNetworkName(Callback cb, const std::string& name) {
  name_ = name;
  std::move(cb).Run(name);
}
```

Good:

```cpp
void MyService::OnGetNetworkName(Callback cb, const std::string& name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  name_ = name;
  std::move(cb).Run(name);
}
```
