---
name: brave/no-narrowing-in-setter-params
description:
  'Setter and function parameter types must match the type of the field they are
  stored in; a narrower integer parameter silently truncates values.'
rubric: correctness
severity: blocking
---

**Rule.** A setter or any function that stores a parameter in a field MUST
declare the parameter with the field's type. Flag a parameter of a narrower or
differently signed integer type (`uint32_t` or `int` stored into `uint64_t`,
`int` into `size_t`) and a floating-point parameter stored into an integer.
Treat it as a bug in wallet, crypto and other security-sensitive code.

**Why.** The narrowing happens at the call site, before the setter runs: callers
passing a value above the parameter's range get it silently truncated, and
nothing in the setter can detect it. In a transaction field, that changes the
amount, fee or validity a user signed.

## Examples

Bad:

```cpp
class Transaction {
  uint64_t invalid_after_ = 0;
  void set_invalid_after(uint32_t value) { invalid_after_ = value; }
};
```

Good:

```cpp
class Transaction {
  uint64_t invalid_after_ = 0;
  void set_invalid_after(uint64_t value) { invalid_after_ = value; }
};
```
