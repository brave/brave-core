---
name: brave/optional-over-bool-and-out-param
description:
  'A function that returns a value which may not exist returns std::optional<T>,
  not a bool success flag with an out parameter.'
rubric: design
severity: should_fix
---

**Rule.** Do not declare a new function as `bool GetFoo(T* out)` (or with a `T&`
out parameter) to report whether it produced a value. Return `std::optional<T>`
instead. When the failure also needs to say why, see
[brave/expected-over-optional-and-error-out-param].

**Why.** With a `bool` and an out parameter, a caller can read the output
without checking the result, and has to declare an uninitialised variable first.
`std::optional` ties the value to its presence and returns it directly.

## Examples

Bad:

```cpp
bool GetHistorySize(int* out_size);
```

Good:

```cpp
std::optional<int> GetHistorySize();
```
