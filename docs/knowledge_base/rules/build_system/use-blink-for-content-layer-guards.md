---
name: brave/use-blink-for-content-layer-guards
description:
  'Code depending on the content layer is guarded by use_blink in GN and
  BUILDFLAG(USE_BLINK) in C++, never by !is_ios or !BUILDFLAG(IS_IOS).'
rubric: correctness
severity: blocking
---

**Rule.** Guard sources, deps and code that need `//content/...` with
`if (use_blink)` in GN and `#if BUILDFLAG(USE_BLINK)` in C++. NEVER use
`!is_ios` or `#if !BUILDFLAG(IS_IOS)` to mean "the content layer is available".

**Why.** `use_blink` is the flag that controls whether the content layer is
built. `!is_ios` is only a proxy for it and is not always equivalent, so code
guarded by it can try to build against a content layer that does not exist.

## Examples

Bad:

```gn
if (!is_ios) {
  sources += [ "content_helper.cc" ]
  deps += [ "//content/public/browser" ]
}
```

```cpp
#if !BUILDFLAG(IS_IOS)
#include "content/public/browser/web_contents.h"
#endif
```

Good:

```gn
if (use_blink) {
  sources += [ "content_helper.cc" ]
  deps += [ "//content/public/browser" ]
}
```

```cpp
#if BUILDFLAG(USE_BLINK)
#include "content/public/browser/web_contents.h"
#endif
```
