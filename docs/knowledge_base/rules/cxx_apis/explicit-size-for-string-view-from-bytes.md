---
name: brave/explicit-size-for-string-view-from-bytes
description:
  "A std::string_view made from a byte buffer or reinterpret_cast pointer must
  carry the buffer's size (base::as_string_view()), never rely on the implicit
  const char* conversion, which calls strlen()."
rubric: safety
severity: blocking
---

**Rule.** NEVER pass a `const char*` obtained from a byte buffer
(`reinterpret_cast<const char*>(data.data())`, a `.data()` of a byte vector or
span) where a `std::string_view` is expected, or construct a `std::string_view`
from such a pointer alone. Convert the whole buffer with
`base::as_string_view(buffer)`, or construct the view with an explicit size.
Check this in particular when a call moves from a `(const char*, size_t)` API to
a `std::string_view` one and the size argument disappears.

**Why.** `std::string_view(const char*)` calls `strlen()`. Byte buffers are not
NUL-terminated, so it reads past the end of the buffer (undefined behaviour, and
a memory-safety bug), or stops early at an embedded zero byte.

## Examples

Bad:

```cpp
std::vector<uint8_t> data = GetBytes();
base::ReadUnicodeCharacter(
    reinterpret_cast<const char*>(data.data()), &i, &code_point);
```

Good:

```cpp
std::vector<uint8_t> data = GetBytes();
base::ReadUnicodeCharacter(base::as_string_view(data), &i, &code_point);
```
