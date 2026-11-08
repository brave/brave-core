---
name: brave/string-view-over-const-char-params
description:
  'String parameters are std::string_view rather than const char*, except
  parameters that must be NUL-terminated, which are base::cstring_view.'
rubric: safety
severity: should_fix
---

**Rule.** Declare a new function parameter that receives string data as
`std::string_view`, not `const char*`. When the function needs the string to be
NUL-terminated, because it passes it on to a C or OS API, declare it as
`base::cstring_view` and pass `.c_str()` on; do not pass a `std::string_view`'s
`.data()` where a NUL-terminated string is expected.

**Why.** A `std::string_view` accepts `std::string`, literals and `const char*`
alike and knows its length, so the callee never needs `strlen()`. A plain
`std::string_view` is not guaranteed to be NUL-terminated, which is what
`base::cstring_view` adds for C APIs.

## Examples

Bad:

```cpp
std::string_view GetDomain(const char* env_from_switch);
```

Good:

```cpp
std::string_view GetDomain(std::string_view env_from_switch);
FILE* OpenFile(const base::FilePath& path, base::cstring_view mode);
```
