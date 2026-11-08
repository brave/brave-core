---
name: brave/no-brave-deps-in-chromium-src
description:
  A chromium_src override avoids including Brave headers or adding //brave/ GN
  deps to upstream targets; it forward-declares a hook implemented in a
  dedicated Brave target.
rubric: design
severity: should_fix
---

**Rule.** A `chromium_src/` override SHOULD NOT `#include` a `brave/...` header,
and an upstream GN target SHOULD NOT gain a dependency on a `//brave/...` target
(by patch or plaster) so that an override compiles. Instead, forward-declare a
free function or factory in the override and call it; implement it in a matching
Brave source file outside `chromium_src/`, in a dedicated Brave target that owns
the Brave includes and dependencies; and make an existing top-level Brave target
(such as `//brave/browser:core`) depend on that target, so the hook links into
the same binary.

**Why.** The override is compiled as part of the upstream target. A Brave
dependency in it is an upstream-to-Brave dependency: it breaks whenever upstream
modularizes, grows the patched target's dependency graph, and forces Brave
dependencies to be patched into upstream GN files.

## Examples

Bad:

```cpp
// chromium_src/chrome/browser/download/download_core_service_impl.cc
#include "brave/browser/download/brave_download_manager_delegate.h"
```

Good:

```cpp
// chromium_src/chrome/browser/download/download_core_service_impl.cc
class ChromeDownloadManagerDelegate;
class Profile;

// Defined in brave/browser/download/brave_download_manager_delegate.cc, and
// declared here so that chrome/browser/download does not depend on Brave
// targets.
std::unique_ptr<ChromeDownloadManagerDelegate>
CreateBraveDownloadManagerDelegate(Profile* profile);

#include <chrome/browser/download/download_core_service_impl.cc>
```
