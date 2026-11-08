---
name: brave/no-content-deps-in-core
description:
  "Code in a component's core/ directory must never depend on //content or on
  the component's content/ subdirectories."
rubric: design
severity: blocking
---

**Rule.** Code under a component's `core/` directory MUST NOT include `content/`
headers, use `content::` types, add GN deps on `//content` or on the component's
own `content/` targets, or allow them in DEPS. Only the reverse is allowed:
`content/` may depend on `core/` and `//content`.

**Why.** iOS builds pull in only `core/`, and iOS has no content layer: a
content dependency in `core/` breaks the iOS build and undoes the split.

## Examples

Bad:

```cpp
// components/brave_shields/core/browser/ad_block_service.cc
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "content/public/browser/browser_thread.h"
```

Good:

```cpp
// components/brave_shields/content/browser/brave_shields_util.cc
#include "brave/components/brave_shields/core/browser/ad_block_service.h"
#include "content/public/browser/browser_thread.h"
```
