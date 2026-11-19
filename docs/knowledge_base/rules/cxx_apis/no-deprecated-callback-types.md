---
name: brave/no-deprecated-callback-types
description:
  'New code must use base::OnceCallback/base::RepeatingCallback and
  base::BindOnce/base::BindRepeating, never the deprecated base::Callback,
  base::Closure or base::Bind.'
rubric: readability
severity: should_fix
---

**Rule.** Do not introduce `base::Callback<>`, `base::Closure` or
`base::Bind()`. Declare callbacks as `base::OnceCallback<>` (or
`base::OnceClosure`) when they run at most once and `base::RepeatingCallback<>`
(or `base::RepeatingClosure`) when they can run several times, and bind them
with the matching `base::BindOnce()` or `base::BindRepeating()`.

**Why.** The old names are deprecated aliases that hide how often a callback may
run. The explicit Once/Repeating types state it in the type, and let the
compiler reject a once-only callback that is run twice.

## Examples

Bad:

```cpp
void FetchData(base::Callback<void(bool)> callback);
FetchData(base::Bind(&FooService::OnFetched, weak_factory_.GetWeakPtr()));
```

Good:

```cpp
void FetchData(base::OnceCallback<void(bool)> callback);
FetchData(base::BindOnce(&FooService::OnFetched, weak_factory_.GetWeakPtr()));
```
