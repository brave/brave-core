---
name: brave/no-gn-private-header-includes
description:
  'Code must not #include a header private to its GN target, but the header its
  owner exports for the purpose; a reviewer never asks for the private include.'
rubric: correctness
severity: should_fix
---

**Rule.** Do not `#include` a header that is private to its GN target: one not
in the target's `public` list, when the including target is not on its `friend`
or `visibility` allowlist. Include instead the header the owner exports for that
purpose, often a `*_test_util.h` or `mock_*.h` from a test-support target.
Before asking for an include of a type a file uses, check the owning target in
`BUILD.gn`: NEVER flag the absence of an include that is private to its target.

**Why.** `gn check` fails on an include of another target's private header, so
the include-what-you-use fix breaks the build. The owner's exported header is
the supported way to reach the type.

## Examples

Bad:

```cpp
// net/dns/host_resolver.h is private to //net/dns:host_resolver.
#include "net/dns/host_resolver.h"
```

Good:

```cpp
// Exported by the test header upstream intends for this.
#include "net/dns/mock_host_resolver.h"
```
