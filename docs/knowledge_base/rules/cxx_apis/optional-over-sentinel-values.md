---
name: brave/optional-over-sentinel-values
description:
  'Represent "no value" with std::optional<T>, never with "", -1 or another
  magic value; only a string whose empty value means nothing but absence may
  stay a plain string tested with .empty().'
rubric: design
severity: blocking
---

**Rule.** NEVER encode "unset", "not found" or "no value" as `""`, `-1`, `0` or
any other magic value of the type. Declare the parameter, field or return value
as `std::optional<T>` and use `std::nullopt` for the missing case. The one
exception: a string for which empty has no meaning other than absence (such as
the result of a lookup that finds nothing) stays a plain string, and callers
test `.empty()`; wrapping it in `std::optional` would create two empty states.

**Why.** A sentinel collides with real values (an empty title the user chose, a
legitimate `-1`), forces every caller to know and repeat the convention, and
cannot be checked by the compiler. `std::optional` makes absence explicit in the
type.

## Examples

Bad:

```cpp
// "" means "unset", though "" is a valid title.
void SetCustomTitle(const std::string& title);
```

Good:

```cpp
// std::nullopt means "unset".
void SetCustomTitle(std::optional<std::string> title);
```
