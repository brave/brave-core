---
name: brave/include-what-you-use
description:
  'A C++ file must #include the header of everything it uses directly, never
  relying on a transitive include; for a type alias, the header declaring the
  alias.'
rubric: correctness
severity: should_fix
---

**Rule.** A file MUST `#include` the header of every type, function, macro and
constant it uses directly, even when another header it includes already pulls
that header in. For a type alias, include the header that declares the alias,
not the headers of the types it names, just as a user of class `B` deriving from
`A` includes `B`'s header and not `A`'s. A header private to its GN target is
the exception (see `brave/no-gn-private-header-includes`): NEVER ask for an
include the target's visibility forbids.

**Why.** A transitive include is an implementation detail of the header that
provides it. When that header drops it, every file that relied on it stops
compiling, often on a platform or configuration the author did not build.

## Examples

Bad:

```cpp
// Uses std::string, but relies on ref_counted.h for <string>.
#include "base/memory/ref_counted.h"
```

Good:

```cpp
#include <string>

#include "base/memory/ref_counted.h"
```
