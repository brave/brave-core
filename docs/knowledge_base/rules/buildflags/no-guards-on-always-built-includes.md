---
name: brave/no-guards-on-always-built-includes
description:
  'An #include of an always-built header stays unconditional even when only code
  inside a #if BUILDFLAG(...) block uses it; only feature-specific headers go
  inside feature guards.'
rubric: readability
severity: should_fix
---

**Rule.** NEVER guard an `#include` just because the code using the header is
inside a `#if BUILDFLAG(...)` block. Only a feature-specific header, one that is
itself gated in GN, goes inside a feature guard. Most Chromium headers
(`web_contents.h`, `tab_interface.h`, `history_service.h`) are always built
whatever the feature flags, and their includes stay unconditional. Before
suggesting a guard, confirm in the relevant `BUILD.gn` `sources` lists and
buildflag conditions that the header is gated.

**Why.** An unconditional include of an always-available header does not break a
disabled-feature build: it merely pulls in a header that configuration does not
use, which is harmless. Wrapping such includes in guards adds churn with no
benefit.

## Examples

Bad:

```cpp
#if BUILDFLAG(ENABLE_AI_CHAT)
#include "content/public/browser/web_contents.h"
#endif
// ...
#if BUILDFLAG(ENABLE_AI_CHAT)
  SearchTabsByContent(...);  // uses web_contents.h
#endif
```

Good:

```cpp
#include "content/public/browser/web_contents.h"
// ...
#if BUILDFLAG(ENABLE_AI_CHAT)
  SearchTabsByContent(...);  // uses web_contents.h
#endif
```
