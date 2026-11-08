---
name: brave/primitives-by-value-not-const-ref
description:
  'Primitive parameters (int, bool, float, enums, pointers) are passed by value,
  never by const reference.'
rubric: perf
severity: should_fix
---

**Rule.** Declare parameters of primitive types (`int`, `bool`, `float`,
`double`, `size_t`, fixed-width integers, `char`, pointers) by value. Do not
write `const int&`, `const bool&`, `const T*&` and the like, including in
callback signatures and lambdas.

**Why.** A primitive fits in a register; passing it by reference passes its
address instead, adding an indirection and an aliasing question for no benefit.

## Examples

Bad:

```cpp
void ProcessItem(const int& id, const bool& enabled);
```

Good:

```cpp
void ProcessItem(int id, bool enabled);
```
