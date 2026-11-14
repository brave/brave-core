---
name: brave/no-buildflag-recheck-in-gated-files
description:
  'A file GN only compiles when a buildflag is on does not include the buildflag
  header just to re-check that same flag with #if BUILDFLAG(...).'
rubric: readability
severity: should_fix
---

**Rule.** When a file is only compiled because its `BUILD.gn` lists it under
`if (enable_feature)`, it does not include the feature's buildflag header to
wrap its code in `#if BUILDFLAG(ENABLE_FEATURE)`. This applies only to the
enable/disable flag that already gates the file: including the header to read
other GN-passed parameters (keys, URLs, numeric limits) is expected. Public
headers may still `static_assert` the flag (see
`brave/static-assert-buildflag-in-public-headers`); this rule covers
implementation files and internal headers strictly behind the GN guard.

**Why.** The file is never compiled when the flag is disabled, so re-checking
that same flag inside it is redundant, and misleading: it suggests the file is
also built with the feature off.

## Examples

Bad:

```cpp
// brave/components/foo/foo_impl.cc, only in sources when enable_foo = true.
#include "brave/components/foo/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_FOO)
void DoFoo() { ... }
#endif
```

Good:

```cpp
// brave/components/foo/foo_impl.cc, only in sources when enable_foo = true.
void DoFoo() { ... }
```
