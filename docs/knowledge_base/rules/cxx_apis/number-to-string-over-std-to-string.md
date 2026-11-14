---
name: brave/number-to-string-over-std-to-string
description:
  "Convert numbers to strings with base::NumberToString(), never
  std::to_string(), which is on Chromium's banned list."
rubric: readability
severity: should_fix
---

**Rule.** Do not call `std::to_string()` (or `std::to_wstring()`). Use
`base::NumberToString()` (or `base::NumberToString16()` for a `std::u16string`)
from `base/strings/string_number_conversions.h`.

**Why.** Chromium bans `std::to_string()`: its output depends on the current
locale, and the `base` conversions are the ones the rest of the codebase uses,
with consistent formatting for every numeric type.

## Examples

Bad:

```cpp
std::string port_str = std::to_string(port);
```

Good:

```cpp
std::string port_str = base::NumberToString(port);
```
