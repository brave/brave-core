---
name: brave/use-make-virtual-not-define-virtual
description:
  'An upstream method that must become virtual is made so with the make_virtual
  plaster rewriter, never with a chromium_src #define adding virtual.'
rubric: design
severity: blocking
---

**Rule.** NEVER add `virtual` to an upstream method with a macro such as
`#define SomeMethod virtual SomeMethod` in a `chromium_src/` header. Write a
`make_virtual` substitution in `rewrite/<path to the header>.yaml`, together
with `drop_final` and `add_friend` substitutions where the subclass needs them.
`make_virtual` also covers the cases a `#define` could not, a pointer or
reference return type and a destructor (quoted, `'~SomeClass'`), so those need
no hand-written patch either.

**Why.** The `#define` rewrites every `SomeMethod` token in the translation
unit, including unrelated declarations pulled in by later includes, and the
change is invisible in the upstream file. The rewriter changes only the one
declaration, and the change shows in the generated patch.

## Examples

Bad:

```cpp
// chromium_src/path/to/original_header.h
#define SomeMethod virtual SomeMethod
#include "src/path/to/original_header.h"
#undef SomeMethod
```

Good:

```yaml
# rewrite/path/to/original_header.h.yaml
substitutions:
  - description: 'Let BraveSomeClass override SomeMethod.'
    make_virtual:
      class_name: SomeClass
      method_name: SomeMethod
```
