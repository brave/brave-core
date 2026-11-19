---
name: brave/patch-internal-linkage-upstream-functions
description:
  An upstream function with internal linkage (static, or in an anonymous
  namespace of a .cc) cannot be replaced through chromium_src; it is changed by
  patching the upstream .cc.
rubric: correctness
severity: should_fix
---

**Rule.** NEVER try to replace an upstream function that has internal linkage, a
`static` function or one in an anonymous namespace of the upstream `.cc`,
through a `chromium_src/` override. Change it with a patch to the upstream `.cc`
instead (one a plaster generates, where a plaster can make the change).
Conversely, NEVER ask for a patch to such a function to be turned into a
chromium_src override.

**Why.** The override mechanism only works for symbols with external linkage. A
function private to the upstream translation unit is not visible outside it, so
an override cannot reach it.

## Examples

Bad:

```cpp
// Upstream chrome/browser/feature.cc
namespace {
void PrivateHelper() { ... }
}

// chromium_src/chrome/browser/feature.cc, an attempt to replace it.
#define PrivateHelper PrivateHelper_ChromiumImpl
#include <chrome/browser/feature.cc>
#undef PrivateHelper
```

Good:

```diff
// patches/chrome-browser-feature.cc.patch
 void PrivateHelper() {
+  if (brave::ShouldSkipHelper()) return;
```
