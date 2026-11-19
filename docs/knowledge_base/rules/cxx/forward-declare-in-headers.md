---
name: brave/forward-declare-in-headers
description:
  'A header forward-declares a type it only uses by pointer or reference and
  leaves the #include to the .cc file, except where the include is already
  needed.'
rubric: perf
severity: should_fix
---

**Rule.** A header includes only what its declarations need. When a type is used
in the header only by pointer or reference, forward-declare it and move its
`#include` to the `.cc` file. NEVER suggest a forward declaration when:

- the type is not `#include`d directly but arrives through the header of a base
  class that must be included anyway (such as `content::WebUI*` through
  `webui_config.h` or `mojo_web_ui_controller.h`), so there is nothing to
  remove;
- the type does not appear in the file at all (check before suggesting);
- the include is for a base class the class inherits from, which cannot be
  forward-declared.

**Why.** Every include in a header is compiled into each file including it, and
every change to the included header rebuilds them all. Forward declarations keep
both compile times and dependencies down.

## Examples

Bad:

```cpp
// my_class.h
#include "components/foo/bar.h"

class MyClass {
  void Use(foo::Bar* bar);
};
```

Good:

```cpp
// my_class.h
namespace foo {
class Bar;
}  // namespace foo

class MyClass {
  void Use(foo::Bar* bar);
};

// my_class.cc
#include "components/foo/bar.h"
```
