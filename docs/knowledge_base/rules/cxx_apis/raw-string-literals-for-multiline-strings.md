---
name: brave/raw-string-literals-for-multiline-strings
description:
  'Embed multiline JavaScript, SQL or other text in C++ with a raw string
  literal R"(...)", not with a concatenation of escaped lines.'
rubric: readability
severity: nit
---

**Rule.** When a C++ string constant spans several lines of text (a script, a
SQL statement, a JSON or HTML snippet), write it as a raw string literal
(`R"(...)"`, or with a delimiter such as `R"JS(...)JS"` when the text contains
`)"`). Do not write it as adjacent literals that each end in `\n"`.

**Why.** A raw literal shows the text as it is: no `\n` and escaped quotes on
every line, no risk of a missing newline or space between fragments, and the
text can be copied in and out of the source unchanged.

## Examples

Bad:

```cpp
const char kScript[] =
    "(function() {\n"
    "  let x = 1;\n"
    "})();";
```

Good:

```cpp
const char kScript[] = R"(
  (function() {
    let x = 1;
  })();
)";
```
