---
name: brave/no-nogncheck-suppressions
description:
  'An include must not carry // nogncheck to silence gn check; the GN dependency
  is fixed, with the right guard, instead.'
rubric: correctness
severity: should_fix
---

**Rule.** Do not add `// nogncheck` to an `#include` to make `gn check` pass.
Fix the underlying GN dependency instead: add the missing dep to the target,
under the same buildflag guard as the include when the include is conditional.

**Why.** `nogncheck` is a code smell that masks a real build dependency issue:
the target uses a header it does not depend on, which breaks in configurations
or build orders where nothing else happens to provide it.

## Examples

Bad:

```cpp
#include "brave/components/foo/bar.h"  // nogncheck
```

Good:

```gn
if (enable_foo) {
  deps += [ "//brave/components/foo" ]
}
```

```cpp
#if BUILDFLAG(ENABLE_FOO)
#include "brave/components/foo/bar.h"
#endif
```
