---
name: brave/sink-params-by-value-not-rvalue-ref
description:
  'A parameter the callee moves into place (a sink parameter) is taken by value,
  not by T&&.'
rubric: readability
severity: should_fix
---

**Rule.** Declare a parameter that the function stores or consumes by moving it
(`name_ = std::move(name)`) as `T name`, not `T&& name`. This does not apply to
move constructors and move assignment operators, nor to forwarding references
(`T&&` where `T` is a deduced template parameter).

**Why.** By value, the function accepts both moved and copied arguments with a
single signature, and the caller still writes `std::move()` to avoid the copy.
An rvalue reference parameter forbids lvalue arguments, which forces callers to
copy explicitly, for no performance gain.

## Examples

Bad:

```cpp
void SetName(std::string&& name) { name_ = std::move(name); }
```

Good:

```cpp
void SetName(std::string name) { name_ = std::move(name); }
```
