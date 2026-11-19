---
name: brave/no-op-lambda-for-function-ref-params
description:
  'Where a base::FunctionRef is expected, a no-op is an explicit lambda;
  base::DoNothing() does not match FunctionRef signatures.'
rubric: correctness
severity: nit
---

**Rule.** Pass an explicit no-op lambda (`[](Item&) {}`) to a parameter of type
`base::FunctionRef<...>`. Do not pass `base::DoNothing()`, and do not suggest
replacing such a lambda with `base::DoNothing()`.

**Why.** `base::DoNothing()` converts only to `base::OnceCallback` and
`base::RepeatingCallback`, so it does not compile where a `base::FunctionRef` is
expected.

## Examples

Bad:

```cpp
service->ForEach(base::DoNothing());  // FunctionRef<void(Item&)>
```

Good:

```cpp
service->ForEach([](Item&) {});
```
