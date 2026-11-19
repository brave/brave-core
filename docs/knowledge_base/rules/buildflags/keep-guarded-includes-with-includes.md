---
name: brave/keep-guarded-includes-with-includes
description:
  "Guarded #includes stay grouped with the other includes at the top of the
  file; one is merged into its code's guard block only when that is the single
  guard block right after the regular includes."
rubric: readability
severity: should_fix
---

**Rule.** Keep each `#if BUILDFLAG(...)`-guarded `#include` with the other
includes at the top of the file. Merge a guarded include into the guard block of
the code using it only when that block is the single guard block immediately
following the regular includes. When more than one buildflag block has its own
guarded include, do NOT merge each include down into its code block: that places
a later include after code.

**Why.** Includes must always come first and only code may follow. Merging every
include into its code block puts the second include after the first block's code
(after a `namespace {...}`), which is incorrect ordering.

## Examples

Bad:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
namespace rewards { ... }
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/components/brave_ads/core/buildflags/buildflags.h"
namespace brave_ads { ... }
#endif
```

Good:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#endif
#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/components/brave_ads/core/buildflags/buildflags.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
namespace rewards { ... }
#endif
#if BUILDFLAG(ENABLE_BRAVE_ADS)
namespace brave_ads { ... }
#endif
```
