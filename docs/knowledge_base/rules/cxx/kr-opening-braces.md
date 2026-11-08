---
name: brave/kr-opening-braces
description:
  'An opening brace goes at the end of the line that opens the block (K&R
  style), not on a line of its own.'
rubric: readability
severity: nit
---

**Rule.** Put the `{` that opens a function, class, namespace or control
statement body at the end of the line that opens it, not on the next line.

**Why.** It is Chromium's style, which `clang-format` applies; code written
another way gets reformatted, and the diff gains noise.

## Examples

Bad:

```cpp
void Foo()
{
  Bar();
}
```

Good:

```cpp
void Foo() {
  Bar();
}
```
