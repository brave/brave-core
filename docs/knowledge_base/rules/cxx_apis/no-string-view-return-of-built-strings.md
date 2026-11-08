---
name: brave/no-string-view-return-of-built-strings
description:
  'A function that builds or concatenates a string internally never returns
  std::string_view to it; it returns std::string or std::optional<std::string>.'
rubric: safety
severity: blocking
---

**Rule.** NEVER return `std::string_view` (or `std::u16string_view`, or a
`base::cstring_view`) from a function whose result is a string it creates: a
local `std::string`, the result of `base::StrCat()`, `base::NumberToString()`,
`base::UTF16ToUTF8()`, `absl::StrFormat()` or a similar call. Return
`std::string` (or `std::optional<std::string>`) instead. Returning a view of a
string literal, a constant with static storage, or a member that outlives the
call is fine.

**Why.** The string the view points into is destroyed when the function returns,
so every caller reads freed memory: a use-after-free that may appear to work
until the memory is reused.

## Examples

Bad:

```cpp
std::string_view BuildUrl(std::string_view host) {
  std::string url = base::StrCat({"https://", host, "/api"});
  return url;  // url destroyed, view dangles!
}
```

Good:

```cpp
std::string BuildUrl(std::string_view host) {
  return base::StrCat({"https://", host, "/api"});
}
```
