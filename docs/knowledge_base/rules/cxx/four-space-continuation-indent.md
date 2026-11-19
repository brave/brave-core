---
name: brave/four-space-continuation-indent
description:
  'A statement wrapped onto further lines indents its continuation lines by 4
  spaces.'
rubric: readability
severity: nit
---

**Rule.** When a statement or declaration does not fit on one line, indent the
lines continuing it 4 spaces from the line it starts on (unless they align with
an open parenthesis, as `clang-format` does).

**Why.** It is Chromium's style, which `clang-format` applies; it sets
continuation lines apart from the 2-space indented body that follows.

## Examples

Bad:

```cpp
std::string result =
  base::StrCat({prefix, separator, suffix});
```

Good:

```cpp
std::string result =
    base::StrCat({prefix, separator, suffix});
```
