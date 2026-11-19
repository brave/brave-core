---
name: brave/str-format-over-string-printf
description:
  'Format strings with absl::StrFormat(), not base::StringPrintf(), which is
  being deprecated in its favour.'
rubric: readability
severity: should_fix
---

**Rule.** New formatted string construction calls `absl::StrFormat()`
(`third_party/abseil-cpp/absl/strings/str_format.h`). Do not add calls to
`base::StringPrintf()`. Pass `std::string` and `std::string_view` arguments
directly for `%s`; `absl::StrFormat()` does not need `.c_str()`.

**Why.** `base::StringPrintf()` is now a thin wrapper around `absl::StrFormat()`
that Chromium plans to remove, so new calls only add to the migration. Both
check the format string against the arguments at compile time.

## Examples

Bad:

```cpp
std::string msg = base::StringPrintf("Error %d: %s", code, desc.c_str());
```

Good:

```cpp
std::string msg = absl::StrFormat("Error %d: %s", code, desc);
```
