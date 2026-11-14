---
name: brave/move-instead-of-copying-values
description:
  'When the original value is not needed anymore, std::move it (into callbacks,
  members and return values) instead of copying or Clone()-ing it, especially
  vectors passed to callback Run() calls.'
rubric: perf
severity: should_fix
---

**Rule.** When a local or parameter is used for the last time, pass it on with
`std::move()` instead of copying it or calling `.Clone()` on it. Check in
particular the arguments of a callback's `.Run()`: a `std::vector`,
`std::string`, `base::Value` or other large object that is not used afterwards
is passed as `std::move(object)`.

**Why.** Passing an lvalue copies it. For a byte buffer, a list of results or a
dictionary, that is a silent extra allocation and a full copy of data that is
about to be destroyed anyway.

## Examples

Bad:

```cpp
std::vector<unsigned char> buffer = BuildData();
std::move(cb).Run(buffer, other_arg);
```

Good:

```cpp
std::vector<unsigned char> buffer = BuildData();
std::move(cb).Run(std::move(buffer), other_arg);
```
