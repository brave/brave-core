---
name: brave/named-functions-over-complex-lambdas
description:
  'A lambda complex enough to obscure the code around it is extracted into a
  named function in the anonymous namespace.'
rubric: readability
severity: nit
---

**Rule.** When a lambda's body is long or intricate enough to make the call
around it hard to follow, move it into a named free function in the file's
anonymous namespace, and bind that (`base::BindOnce(&ProcessResult)`). Short
lambdas stay inline.

**Why.** A named function says what the code does at the call site, and can be
read, and tested, on its own; a long lambda hides the structure of the function
containing it.

## Examples

Bad:

```cpp
DoSomething(base::BindOnce([](int a, int b, int c) {
  // 20 lines of complex logic...
}));
```

Good:

```cpp
namespace {

void ProcessResult(int a, int b, int c) {
  // 20 lines of complex logic...
}

}  // namespace

DoSomething(base::BindOnce(&ProcessResult));
```
