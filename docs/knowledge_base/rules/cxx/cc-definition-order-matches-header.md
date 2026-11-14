---
name: brave/cc-definition-order-matches-header
description:
  'Definitions in a .cc file appear in the same order as their declarations in
  the corresponding header.'
rubric: readability
severity: nit
---

**Rule.** Define the functions and methods of a `.cc` file in the order the
corresponding `.h` file declares them. A method a change adds to the header is
defined at the matching position in the `.cc` file, not appended at its end.

**Why.** With the same order in both files, a reader finds a definition from its
declaration, and the reverse, without searching.

## Examples

Bad:

```cpp
// foo.h declares Start(), then Stop(), then Reset().
void Foo::Reset() { ... }
void Foo::Start() { ... }
void Foo::Stop() { ... }
```

Good:

```cpp
void Foo::Start() { ... }
void Foo::Stop() { ... }
void Foo::Reset() { ... }
```
