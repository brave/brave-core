---
name: brave/do-nothing-for-no-op-callbacks
description:
  'Pass base::DoNothing() where a callback that does nothing is needed, not an
  empty lambda.'
rubric: readability
severity: nit
---

**Rule.** When an API requires a callback and the caller has nothing to do when
it runs, pass `base::DoNothing()` (or `base::DoNothingAs<Signature>()` when the
signature must be spelled out). Do not bind an empty lambda such as
`base::BindOnce([](const std::string&) {})`.

**Why.** `base::DoNothing()` adapts to any callback signature and says at a
glance that the result is deliberately ignored; an empty lambda repeats the
signature and makes the reader check that its body is really empty.

## Examples

Bad:

```cpp
service->DoAsync(base::BindOnce([](const std::string&) {}));
```

Good:

```cpp
service->DoAsync(base::DoNothing());
```
