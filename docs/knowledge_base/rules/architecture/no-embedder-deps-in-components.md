---
name: brave/no-embedder-deps-in-components
description:
  'Code in components/ must never include, depend on or allow in DEPS the
  embedder layer: brave/browser/, chrome/ or //android_webview.'
rubric: design
severity: blocking
---

**Rule.** Code under `components/` (and `chromium_src/components/`) MUST NOT
include a header from, add a GN dependency on, or open a DEPS `include_rules`
entry to the embedder layer: `brave/browser/`, `chrome/` (`//chrome`),
`//android_webview` or `ios/chrome/`. Dependencies flow downward only:
components may depend on `//content`, `//net`, `//base` and other components.
When component code needs something from the browser layer, inject it (see
`brave/abstract-interface-for-browser-access-from-components`).

**Why.** Components are a lower layer, shared by several embedders. A dependency
on the embedder is a Chromium layering violation: it creates dependency cycles,
and the component no longer builds for an embedder that lacks that code.

## Examples

Bad:

```cpp
// components/brave_shields/core/browser/ad_block_service.cc
#include "brave/browser/brave_browser_process.h"
#include "chrome/browser/profiles/profile.h"
```

Good:

```cpp
// components/brave_shields/core/browser/ad_block_service.cc
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
```
