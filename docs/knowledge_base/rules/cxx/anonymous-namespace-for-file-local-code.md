---
name: brave/anonymous-namespace-for-file-local-code
description:
  'Functions, classes and constants used only within a .cc file go in an
  anonymous namespace, not in a named namespace or behind static.'
rubric: readability
severity: should_fix
---

**Rule.** Wrap every function, class, constant and type that is used only within
one `.cc` file in an unnamed namespace (`namespace { ... }  // namespace`). Do
not give such a free function internal linkage with `static` instead, and do not
leave it in the file's named namespace, where it is visible to other files.

**Why.** Both an unnamed namespace and `static` give internal linkage, but
Chromium style wraps everything local to a `.cc` file in an unnamed namespace,
which also covers classes and types. A helper left in a named namespace can
clash with a symbol of the same name in another file.

## Examples

Bad:

```cpp
static void InternalHelper() { ... }
```

Good:

```cpp
namespace {

void InternalHelper() { ... }

}  // namespace
```
