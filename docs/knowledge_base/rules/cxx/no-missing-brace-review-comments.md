---
name: brave/no-missing-brace-review-comments
description:
  'Bodies of if, else, for and while are always braced, but a reviewer never
  flags missing braces, which clang-format inserts.'
rubric: readability
severity: nit
---

**Rule.** Authors brace the body of every `if`, `else`, `for` and `while`, even
a single statement. A reviewer NEVER reports a missing brace: Chromium's
`clang-format` configuration inserts them.

**Why.** Braces keep a statement added later from silently falling outside the
conditional, and the formatter adds them anyway, so a review comment about them
is noise.

## Examples

Bad:

```cpp
// Review comment: "Please add braces."
if (!service)
  return;
```

Good:

```cpp
// No comment; clang-format braces it.
if (!service)
  return;
```
