---
name: brave/no-includes-added-by-patches
description:
  A header an upstream source needs for Brave's code must be included from its
  chromium_src override, never added by a patch.
rubric: design
severity: should_fix
---

**Rule.** NEVER add an `#include` (or `#import`) line to an upstream C++ file
through a hand-written patch. Put the include in the file's `chromium_src/`
override, creating one if needed, ahead of the line that re-includes the
upstream source.

**Why.** Patches are harder to maintain and more likely to conflict than
overrides, and an include list is where upstream edits most often: an include
patched into it conflicts on rebases for no functional reason.

## Examples

Bad:

```diff
// patches/chrome-browser-some_file.cc.patch
+#include "brave/components/my_feature/my_header.h"
```

Good:

```cpp
// chromium_src/chrome/browser/some_file.cc
#include "brave/components/my_feature/my_header.h"

#include <chrome/browser/some_file.cc>
```
