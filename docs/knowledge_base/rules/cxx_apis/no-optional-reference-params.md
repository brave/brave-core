---
name: brave/no-optional-reference-params
description:
  'Never take std::optional<T>& or const std::optional<T>& as a function
  parameter; take std::optional<T> by value when storing it, or
  base::optional_ref<T> for a non-owning optional reference.'
rubric: perf
severity: blocking
---

**Rule.** NEVER declare a parameter as `const std::optional<T>&` or
`std::optional<T>&`. Take `std::optional<T>` by value when the function stores
or consumes it (and move it into place), or `base::optional_ref<const T>` when
it only reads an optional value it does not own. This applies to parameters
only; an accessor may return `const std::optional<T>&` to a member.

**Why.** A caller holding a plain `T` (or `std::nullopt`) has to materialise a
temporary `std::optional<T>`, copying the `T`, just to bind the reference, and
the signature does not tell whether the callee keeps it. `base::optional_ref`
binds to a `T`, an optional or `std::nullopt` without any copy.

## Examples

Bad:

```cpp
void Process(const std::optional<std::string>& value);
```

Good:

```cpp
void Process(std::optional<std::string> value);             // stores it
void Process(base::optional_ref<const std::string> value);  // only reads it
```
