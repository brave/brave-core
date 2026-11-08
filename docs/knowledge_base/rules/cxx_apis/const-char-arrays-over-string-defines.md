---
name: brave/const-char-arrays-over-string-defines
description:
  'String constants are declared as extern const char[] (defined in the .cc
  file), not as #define macros, unless the value is passed in from GN.'
rubric: readability
severity: should_fix
---

**Rule.** Do not `#define` a string constant (`#define MY_URL "..."`). Declare
it inside the component's namespace as `extern const char kMyUrl[];` in the
header and define it in the `.cc` file. The one exception is a value that GN
passes in through `defines`, which can only be a macro.

**Why.** Macros ignore namespaces and scopes, so a string macro can clash with
any other identifier of the same name anywhere it is included, and it is
invisible to the debugger and to code search's symbol cross-references.

## Examples

Bad:

```cpp
#define MY_URL "https://example.com"
```

Good:

```cpp
// In the .h:
extern const char kMyUrl[];
// In the .cc:
const char kMyUrl[] = "https://example.com";
```
