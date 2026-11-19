---
name: brave/named-constants-for-json-keys
description:
  'C++ code reading or writing JSON object properties uses named constants for
  the key strings, not inline string literals.'
rubric: readability
severity: nit
---

**Rule.** When a `base::Value::Dict` built from or for JSON is accessed with
`Find*()`, `Set()`, `Remove()` or `Extract()`, the key is a named constant
(`constexpr char kDisplayName[] = "display_name";`), defined once and shared by
the code that reads and writes it. Do not repeat the key as a string literal at
each access.

**Why.** A misspelled literal key compiles and silently reads nothing; a
constant is checked by the compiler, found by code search, and renamed in one
place.

## Examples

Bad:

```cpp
auto* name = dict.FindString("display_name");
auto* url = dict.FindString("endpoint_url");
```

Good:

```cpp
constexpr char kDisplayName[] = "display_name";
constexpr char kEndpointUrl[] = "endpoint_url";
auto* name = dict.FindString(kDisplayName);
auto* url = dict.FindString(kEndpointUrl);
```
