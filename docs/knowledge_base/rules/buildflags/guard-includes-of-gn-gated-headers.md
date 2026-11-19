---
name: brave/guard-includes-of-gn-gated-headers
description:
  'An #include of a header that GN only builds under a buildflag goes inside a
  #if BUILDFLAG(...) guard for that flag; a guard is only ever suggested with a
  buildflag that exists.'
rubric: correctness
severity: blocking
---

**Rule.** When a header is conditionally available (its target or sources are
gated behind a buildflag in GN, or the header sits behind its own buildflag),
its `#include` MUST be inside a `#if BUILDFLAG(...)` block for that same flag.
Before suggesting such a guard, verify the buildflag is defined in the codebase
(in a `buildflags.gni` or by existing `BUILDFLAG(...)` uses): NEVER fabricate or
assume a buildflag name. If the feature has no buildflag, it may use
`base::FeatureList` runtime checks or have no compile-time guard at all.

**Why.** An unconditional include of a GN-gated header breaks the build when the
feature is disabled, because the header does not exist in that configuration. A
guard naming a buildflag that does not exist does not compile.

## Examples

Bad:

```cpp
// extension_web_ui.h is only built when ENABLE_EXTENSIONS is set.
#include "chrome/browser/extensions/extension_web_ui.h"
// ...
#if BUILDFLAG(ENABLE_EXTENSIONS)
  IsChromeURLOverridden(...);
#endif
```

Good:

```cpp
#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_web_ui.h"
#endif
// ...
#if BUILDFLAG(ENABLE_EXTENSIONS)
  IsChromeURLOverridden(...);
#endif
```
