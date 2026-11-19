---
name: brave/expected-over-optional-and-error-out-param
description:
  'A function that can fail and must report why returns base::expected<T, E>,
  not std::optional<T> with a separate error out-parameter.'
rubric: design
severity: should_fix
---

**Rule.** Do not declare a new function that returns `std::optional<T>` (or
`bool`) and reports failure details through an out parameter such as
`std::string* error`. Return `base::expected<T, E>` with the error type `E` (a
string, an enum or a struct), and return `base::unexpected(error)` on failure.

**Why.** With a separate out parameter, the error can be left unset on failure
or set on success, and callers must declare it up front. `base::expected`
carries exactly one of the value or the error, and works with `ASSIGN_OR_RETURN`
and `RETURN_IF_ERROR`.

## Examples

Bad:

```cpp
std::optional<Result> Parse(const std::string& input, std::string* error);
```

Good:

```cpp
base::expected<Result, std::string> Parse(const std::string& input);
```
