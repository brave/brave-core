---
name: brave/bind-weak-ptr-to-method-not-lambda
description:
  'Async callbacks bind a WeakPtr directly to a member function; they must not
  pass a WeakPtr into a lambda.'
rubric: correctness
severity: should_fix
---

**Rule.** When an asynchronous callback needs a `base::WeakPtr`, bind it as the
receiver of a member function
(`base::BindOnce(&MyService::OnDone, weak_ptr_factory_.GetWeakPtr(), ...)`). Do
not pass the `WeakPtr` into a lambda (as a bound parameter or a capture) and
check it there.

**Why.** A callback whose receiver is a `WeakPtr` is cancelled automatically
once the pointer is invalidated. A lambda gets no such cancellation: it runs
anyway and relies on a hand-written check, which is easy to get wrong or forget.

## Examples

Bad:

```cpp
auto weak_this = weak_ptr_factory_.GetWeakPtr();
rpc_->GetNetworkName(base::BindOnce(
    [](base::WeakPtr<MyService> self, Callback cb, const std::string& name) {
      if (!self) return;
      std::move(cb).Run(name);
    }, weak_this, std::move(callback)));
```

Good:

```cpp
rpc_->GetNetworkName(base::BindOnce(
    &MyService::OnGetNetworkName,
    weak_ptr_factory_.GetWeakPtr(),
    std::move(callback)));
```
